#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "pbft-node.h"
#include "stdlib.h"
#include "ns3/ipv4.h"
#include <ctime>
#include <map>
#include <string>
#include <iostream>
#include <sstream>



int n;
int nMax=100000-1;
int v;
int val;
int n_round;
uint8_t *Faulty;
int ViewChangeNbre;

static float TxStart[100000]; // max transaction dans la simulation est===100000=== transactions MAXimuim
int Tended[100000]; // transacrion ended no need to consider timeout 


///////////////////////
enum PBFT_TYPE_T
{
    PBFT,           // 0       
    PBFT_Cluster,       // 1       
    RC_PBFT         // 2
};

enum Arrival_model 
{
    constant_period,           // 0       
    poissonian_arrival,       // 1       
};
//////////////////////////////////////

// simulation parameters .............................................
int TimeStopEvents= 1.5; // stop time in seconds 
float CV_timeout= 0.05 ;  // in seconds  view change timeout  
int size = 350; // by default 100, minimium is 4X6=24
int limit_view_change =30; // limits of view changes, system blockage ....

int Ncl=4;  // Cluster size  (Ncl < N) because N nodes = {N-1 nodes, client node 0 }   
int NFault=3;
enum Arrival_model Arr_model=poissonian_arrival;
float Block_creation_period=2;           //   period of block/transaction per  second (CBR : constant bit rate)
float lambda=400 ;						// lambda block per second (VBR: varibale bit rate) poissonian arrival 
 
enum PBFT_TYPE_T pbft_Type=RC_PBFT ;  // PBFT,  PBFT_Cluster, RC_PBFT 
bool Display_Creation_block_time= false; // true or false


//.............................................................................


struct  tupleDouble { // temps de réponse en fonction du temps 
  double t;
  double tr;
};

std::list<tupleDouble> list_reponse_Times;
std::list<float> list_creation_Times;



// declared as static 
std::map<int, std::vector<int>>    ns3::PbftNode::m_clusters;
std::map<int, int>                 ns3::PbftNode::m_clusters_leaders;



namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("PbftNode");

NS_OBJECT_ENSURE_REGISTERED (PbftNode);



TypeId
PbftNode::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::PbftNode")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<PbftNode> ()
    ;

    return tid;
}

PbftNode::PbftNode(void) {

}

PbftNode::~PbftNode(void) {
    NS_LOG_FUNCTION (this);
}



static string Convert4IntToString (int a, int b, int c, int d)
{
   
  string s;
  stringstream out;
  out <<a<<","<<b<<","<<c<<","<<d;
  s = out.str();
  return s;  
}

static void ConvertStringTo4Int (string s,int *Tint4 )
{
       stringstream check1(s);
    string st;
     
    // Tokenizing w,r,t, sprate ','
    //while(getline(check1, st, ','))
    for (int i=0;i<4;i++)
    {
        getline(check1, st, ',');
	//	NS_LOG_INFO("\n st= "<<st);
		
        Tint4[i]=stoi(st);
        
    }
 
}

////////////////////////
void copy_4Int_string_uint8_t (string s,uint8_t *data ){
    
    std::stringstream check1(s);
    string st;
    int len=0;
    for (int i=0;i<4;i++)
     { getline(check1, st, ',');
        len+=st.length(); }
    len+=3; // sperators
    for (int i=0;i<len;i++)
      data[i]=s[i];
}



// random delay could be in simulation:  3 - 6 ms 
float 
getRandomDelay() {
  return 0;
  //return ((rand() % 10) * 1.0 + 3) / 1000; 

}

void init_TxStart() {
  for (int i=0;i<100000;i++) /// 100000 max transactions
   Tended[i]=0;
}


void
printVector(std::vector<int> vec) {
	
	for (auto const &i: vec) 
	{
        NS_LOG_INFO(i<< " ");
    }
}


// poission process for packet/transaction arrivals/creations
float next_time_interval (float lambda )
{
	float n, _inter_event_time;
	//_event_time*= 1000.0;
	
	 n=((rand()%100)*1.0)/100.0;
	 _inter_event_time = -log(1.0 - n) / lambda;
	//_event_time = _event_time + _inter_event_time;
	
	return (_inter_event_time);
	
	
}


void printCluster(std::vector<int> vec) {
	
	std::stringstream ss;
    ss <<"Cluster: ";
	for (auto const &i: vec) 
	  {
		ss<<" "<<i;
	  }
    NS_LOG_INFO(ss.str());

}


void DisplayList( std::list<float>const &listTimes, std::list<tupleDouble>const &list, int TimeStopEvents)
{
    //std::list<double>::iterator it;
	NS_LOG_INFO ("\n==============================");
	NS_LOG_INFO ("==============================");
	if ( Display_Creation_block_time== true ){
	   NS_LOG_INFO ("\n\n ==============================\n\n");
	   NS_LOG_INFO (" Time of block creations or arrivials=");
	   for (auto const &i: listTimes) 
		  if (i<=TimeStopEvents)	NS_LOG_INFO (""<< i);
	}
	
	
	NS_LOG_INFO ("\n\n ==============================\nResponse Time list:\n");
	NS_LOG_INFO (" Time of validations=");
	for (auto const &i: list) 
		if (i.t<=TimeStopEvents)	NS_LOG_INFO (""<< i.t);
	
	NS_LOG_INFO (" Response Time=");
    for (auto const &i: list) 
		if (i.t<=TimeStopEvents) NS_LOG_INFO (""<< i.tr);
	
    NS_LOG_INFO ("\n==============================\n");
    NS_LOG_INFO ("\n ==============================\n");
}


double CalculateAvg( std::list<tupleDouble>const &list)
{
    double avg = 0;
    //std::list<double>::iterator it;
	for (auto const &i: list) avg+=i.tr;
   // for(it = list->begin(); it != list->end(); it++) avg += *it;
    avg /= list.size();
	
	return avg;
}

double CalculateMax( std::list<tupleDouble>const &list)
{
    double max = 0;
	
    //std::list<double>::iterator it;
	for (auto const &i: list) 
		if (max <i.tr) max=i.tr;
  
	
	return max;
}

double CalculateMin( std::list<tupleDouble>const &list)
{
    double min = 10000000;
	
    //std::list<double>::iterator it;
	for (auto const &i: list) 
		if (min >i.tr) min=i.tr;
  
	
	return min;
}

double CalculateAvg( std::list<tupleDouble>const &list,int TimeStopEvents)
{
    double avg = 0;
    //std::list<double>::iterator it;
	for (auto const &i: list) 
		  if (i.t<=TimeStopEvents) avg+=i.tr;
   // for(it = list->begin(); it != list->end(); it++) avg += *it;
    avg /= list.size();
	
	return avg;
}

double CalculateMax( std::list<tupleDouble>const &list,int TimeStopEvents)
{
    double max = 0;
	
    //std::list<double>::iterator it;
	for (auto const &i: list) 
		if (i.t<=TimeStopEvents) if (max <i.tr) max=i.tr;
  
	
	return max;
}

double CalculateMin( std::list<tupleDouble>const &list, int TimeStopEvents)
{
    double min = 10000000;
	
    //std::list<double>::iterator it;
	for (auto const &i: list) 
		if (i.t<=TimeStopEvents) if (min >i.tr) min=i.tr;
  
	
	return min;
}


///////////////////////////////////// 
void 
PbftNode::random_cluster(int N,int Ncl, int nBlock)   // the cluster do not contains client node (node0)
{	
	// static std::map<int, vector<int>>    m_clusters;
	
    if (Ncl>N) {NS_LOG_INFO ("\n Error Ncl > N"); exit(-3);}
	if (Ncl==N) {NS_LOG_INFO ("\n Error Ncl = N could be lower than N, because  N={N-1 nodes, client node}"); exit(-3);}
	
	std::vector<int> Vn;
	int n;
	for (int i=0;i<Ncl;i++)
	 {
		 do{
	        n=( int)((rand()%(N-1))*1.0+1); // random in 1..N-1  and  0 for client !!!
		 } while (count(Vn.begin(),Vn.end(),n)!=0);
	  Vn.push_back (n);
	 }
	 
   PbftNode::m_clusters[nBlock]=Vn;
   PbftNode::m_clusters_leaders[nBlock]= Vn[( int)((rand()%(Ncl-1))*1.0)]; // random leader from the cluster 0..Ncl-1
   
   //NS_LOG_INFO ("\n n="<<nBlock<<" ");
   //printVector(Vn);
	
}



static  uint8_t* generateTX (int num, int leader)
{	
  string s=Convert4IntToString (REQUEST, 0, n, leader);/////////////////////
	
  int size = num * sizeof(uint8_t);
  uint8_t *data = (uint8_t *)std::malloc (size);
   
  int i;
  for (i = 0; i < size; i++) {
    data[i] = '1';
  }
  data[i] = '\0';
  /*
  // NS_LOG_INFO("   " << data);  data[0] = REQUEST;   data[1] = 0;//intToChar(v);  data[2] = n;  data[3] = leader; //....
*/

 for (i = 0; i < (int)s.length(); i++) {
	 data[i]=s[i];
 }
 data[i] = '\0';

  return data;
}


///////////////
 uint8_t * generate_Faulty_Nodes1 (int num, int nfaulty)
{
  int size = num * sizeof(uint8_t);
  uint8_t *faulty = (uint8_t *)std::malloc (size);
  int i;
  faulty[0] = '0'; // noeud 0 est le noeud client 
   for (i = 1; i <=nfaulty; i++) 
    faulty[i] = '1';
  
  for (; i < size; i++) 
    faulty[i] = '0';
  
  return faulty;
}

static int NbreTansactions( std::list<tupleDouble>const &list, int TimeStopEvents){
	
	int nbre=0;
   for (auto const &i: list) 
		if (i.t<=TimeStopEvents) nbre++;
        else break;

return (nbre);	

}

			

void PbftNode::display_stat()
{
             DisplayList(list_creation_Times,list_reponse_Times, TimeStopEvents);
			 if (pbft_Type==PBFT_Cluster) NS_LOG_INFO(" PBFT Type = PBFT_Cluster");
			 if (pbft_Type==PBFT) NS_LOG_INFO(" PBFT Type = PBFT");
			 if (pbft_Type==RC_PBFT) NS_LOG_INFO(" PBFT Type = RC_PBFT");
			
			 NS_LOG_INFO("N ="<<N<<" ={"<<(N-1)<<" nodes, client node 0}, -- Ncluster= " << Ncl );
			 NS_LOG_INFO("=== lambda (block arrival or creation per second)="<< lambda );
			 NS_LOG_INFO("=== Faulty nodes number= "<< NFault<< "== block size="<<size);
			 int nbretrans= NbreTansactions(list_reponse_Times, TimeStopEvents);
			 
			 NS_LOG_INFO("== Tansction number (in simulation time) ="<<nbretrans<<" -- TPS (transactions per second) Average ="<<nbretrans/TimeStopEvents);
			 
			 float avg= CalculateAvg(list_reponse_Times,TimeStopEvents);
			 float Min= CalculateMin(list_reponse_Times,TimeStopEvents);
			 float Max= CalculateMax(list_reponse_Times,TimeStopEvents);
			 NS_LOG_INFO("=== Client node "<< m_id << " Response Time (Average)="<<avg<< " (Min)="<<Min<< " (Max)="<<Max<<" ");
			 NS_LOG_INFO("Number of view changes " << ViewChangeNbre << "\n Simulation END..." );
}

///////////


void 
PbftNode::StartApplication ()            
{
    v = 1;              //  view number
    n = 0;              //   transaction or block number 
    leader = 1;            //  leader noeud 1
    
    block_num = 0;
	ViewChangeNbre=0;
   
    n_round = 0;                      

    val = m_id; /////

   srand(time(0)); /// random

   init_TxStart(); // vecteur sur le temps start des transactions 

    // socket
    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket (GetNode (), tid);

        InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 7071);
        m_socket->Bind (local);          
        m_socket->Listen ();
    }
    m_socket->SetRecvCallback (MakeCallback (&PbftNode::HandleRead, this));
    m_socket->SetAllowBroadcast (true);

    std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();
    
    while(iter != m_peersAddresses.end()) {
        // NS_LOG_INFO("node"<< m_id << *iter << "\n");
        TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
        Ptr<Socket> socketClient = Socket::CreateSocket (GetNode (), tid);
        socketClient->Connect (InetSocketAddress(*iter, 7071));
        m_peersSockets[*iter] = socketClient;
        iter++;
    }
    
		Faulty= generate_Faulty_Nodes1 (N, NFault); // sof ++ si 1 seule le noeud 1 est défaillant 
		
		
    if (Arr_model==constant_period) Simulator::Schedule(Seconds(Block_creation_period), &PbftNode::SendBlock, this);
	else {  float tt= next_time_interval (lambda ); // poissonian_arrival
	        Simulator::Schedule(Seconds(tt), &PbftNode::SendBlock, this);
	     }
	 
        // n++;
    // }
}

void 
PbftNode::StopApplication ()
{
    // printVector(values);
	 if (m_id == (unsigned int) client) {
		     display_stat();
			 
			 /*
		     DisplayList(list_creation_Times,list_reponse_Times, TimeStopEvents);
			 if (pbft_Type==PBFT_Cluster) NS_LOG_INFO(" PBFT Type = PBFT_Cluster");
			 if (pbft_Type==PBFT) NS_LOG_INFO(" PBFT Type = PBFT");
			 if (pbft_Type==RC_PBFT) NS_LOG_INFO(" PBFT Type = RC_PBFT");
			 NS_LOG_INFO("N ="<<N<<" ={"<<(N-1)<<" nodes, client node 0}, -- Ncluster= " << Ncl );
			 
			 int nbretrans= NbreTansactions(list_reponse_Times, TimeStopEvents);
			 
			 NS_LOG_INFO("== Tansction number (in simulation time) ="<<nbretrans<<" -- TPS (transactions per second) Average ="<<nbretrans/TimeStopEvents);
			 
			// NS_LOG_INFO("== Tansction number="<<list_reponse_Times.size()<<" -- TPS (transactions per second) Average ="<<list_reponse_Times.size()/TimeStopEvents);
			 
			 
			 float avg= CalculateAvg(list_reponse_Times, TimeStopEvents);
			 float Min= CalculateMin(list_reponse_Times, TimeStopEvents);
			 float Max= CalculateMax(list_reponse_Times, TimeStopEvents);
			 NS_LOG_INFO("=== Client node "<< m_id << " Response Time (Average)="<<avg<< " (Min)="<<Min<< " (Max)="<<Max<<" ");
			 NS_LOG_INFO("Number of view changes " << ViewChangeNbre << "\n Simulation END..." );
			 */
	 }
}





void 
PbftNode::HandleRead (Ptr<Socket> socket)
{   
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom (from)))
    {
        //socket->SendTo(packet, 0, from);
        if (packet->GetSize () == 0)
        {   //EOF
            break;
        }
        if (InetSocketAddress::IsMatchingType (from))
        {
            std::string msg = getPacketContent(packet, from);

          			
           // uint8_t data[4];  // original
		    uint8_t *data = (uint8_t *)std::malloc (size); // a verifier  l impact d utiliser size dans les delais des paquets
			
			
			int TDataint4[4];
			ConvertStringTo4Int (msg,TDataint4);
			v=TDataint4[1];
		
			
			 if (TDataint4[0]!=VIEW_CHANGE && Faulty[m_id]=='1' && m_id!= (unsigned int)client) return ; // un noeud defaillant bloque les reponses et messages....
			//if (Faulty[GetNode ()->GetId ()]=='1' && m_id!= (unsigned int)client) return ;
			
            switch (TDataint4[0])
            {
                case REQUEST:
				 {   
				 int num = TDataint4[2];
				 
				 int leader ; 
				 leader= TDataint4[3];
				// PbftNode::m_clusters_leaders[n]=charToInt(msg[3]);
				 
				 //  TxStart[num]= Simulator::Now ().GetSeconds (); // ajout sof ++
				// NS_LOG_INFO("Request  id"<< m_id <<" leader="<<leader <<"\n");
				  
				  if (m_id == (unsigned int) leader) 
				    {
                    
                    TDataint4[0] = PRE_PREPARE;
					v=TDataint4[1];
					
                 //   data[1] = msg[1];// v             //  data[2] = msg[2];// n            // data[3] = msg[3];// value
                    ////////// TDataint4[1] =v, TDataint4[2]=n, TDataint4[3]= value
					string s= Convert4IntToString (TDataint4[0], TDataint4[1], TDataint4[2], TDataint4[3]); 
					
					copy_4Int_string_uint8_t (s,data);
					

                    // value
                    tx[num].val = TDataint4[3];
					
					 //NS_LOG_INFO("Leader node"<< m_id << " Send Pre-Prepaper messages  ");
                    //NS_LOG_INFO(" Leader node"<< m_id << " ");
					//printCluster(PbftNode::m_clusters[num]); 
					//NS_LOG_INFO("");
					//SendToAllCluster(data, PbftNode::m_clusters[1]);
                     SendToAllCluster(data, PbftNode::m_clusters[num]);
				   //Send(data); 
				   }
				 }
				  
                    break;
				
				case PRE_PREPARE:           
                {   //if ( m_id == (unsigned int) client) break; 
				 
				    TDataint4[0] = PREPARE;
					string s= Convert4IntToString (TDataint4[0], TDataint4[1], TDataint4[2], TDataint4[3]); 
					copy_4Int_string_uint8_t (s,data);
					
                   // data[0] = intToChar(PREPARE);     //data[1] = msg[1];// v     //data[2] = msg[2];// n    //data[3] = msg[3];  // value
   
                    //int num = charToInt(msg[2]); 
					int  num =TDataint4[2];

                    // value
                    //tx[num].val = charToInt(msg[3]);
					tx[num].val =TDataint4[3];
					//NS_LOG_INFO(" node"<< m_id << " Send Pre-Prepaper message  n="<<num<<" ");
                    
				    //Send(data); 
					SendToAllCluster(data, PbftNode::m_clusters[num]);

				   }
				  
                    break;
                
                case PREPARE:           
                {       	
                   	//if ( m_id == (unsigned int) client) break; 	
					// int index = charToInt(msg[2]);
                    int index = TDataint4[2];
					
					int Nlimit;
					 if (pbft_Type== PBFT ) Nlimit=N-1;
					   else Nlimit=Ncl; // PBFT_Cluster , RC_PBFT
					
					tx[index].prepare_vote++;
					
					//NS_LOG_INFO(" node"<< m_id << " index="<<index);
					//NS_LOG_INFO("Prepaper node"<< m_id << " index="<<index << " votes: " << tx[index].prepare_vote);	
					
                        
                    
                    // SUCCESS1, COMMIT
                    if (tx[index].prepare_vote>0 && tx[index].prepare_vote >= ((2 * Nlimit) / 3) ) {
					
                        //data[0] = intToChar(COMMIT);    //data[1] = msg[1]; // v  //data[2] = msg[2];// n	   //data[3] = intToChar(SUCCESS1); 
						 
						 TDataint4[0] = COMMIT; TDataint4[3] = SUCCESS1;
						string s= Convert4IntToString (TDataint4[0], TDataint4[1], TDataint4[2], TDataint4[3]); 
						copy_4Int_string_uint8_t (s,data);
						 
						  tx[index].prepare_vote = -1000;
						  
                      //Send(data);
					  SendToAllCluster(data, PbftNode::m_clusters[index]);
				   //   NS_LOG_INFO(">>>>prepare voted  node "<< m_id << " index="<<index << " vote=" << tx[index].prepare_vote);
                       
                    }
                    break;
                 }
                
                case COMMIT:           
                  {   
                    if ( m_id == (unsigned int) client) break; 
					int Nlimit;
					 if (pbft_Type== PBFT ) Nlimit=N-1;
					   else Nlimit=Ncl; // PBFT_Cluster , RC_PBFT
					
                   // int index = charToInt(msg[2]);
					int index = TDataint4[2];
					//NS_LOG_INFO(" node"<< m_id << " Send COMMIT message  n="<<charToInt(msg[2])<<" ");
					
                   // NS_LOG_INFO("commit node"<< m_id << " index="<<index <<" commit_vote=" << tx[index].commit_vote);
                    tx[index].commit_vote++;
                    
                    if (tx[index].commit_vote>0 && tx[index].commit_vote > 2 * Nlimit / 3) {
                        //data[0] = intToChar(REPLY);    //data[1] = msg[1];//intToChar(v);  //data[2] =  msg[2];    //intToChar(n);   //  //data[3] = SUCCESS1;       // n
						TDataint4[0] = REPLY; TDataint4[3] = SUCCESS1;
						string s= Convert4IntToString (TDataint4[0], TDataint4[1], TDataint4[2], TDataint4[3]); 
						copy_4Int_string_uint8_t (s,data);
						
                        // Send(data);
                        tx[index].commit_vote = -1000;
						v=TDataint4[1];

                       // NS_LOG_INFO("node "<< m_id << " in view " << v << " finish n=" << n << "th block index="<<index<<" :submit, at time " << Simulator::Now ().GetSeconds () << "s,"); //value is " << values[block_num] << "\n");
                       
                        NS_LOG_INFO("node "<< m_id << " in view " << v << " finish " << index << "th block  :submit, at time " << Simulator::Now ().GetSeconds () << "s,");
						//block_num++;
                        // n = n + 1;
					   SendTo(data,client);
					   
					   if (pbft_Type== RC_PBFT) {
						   TDataint4[0] = REPLY_OtherNodes; TDataint4[3] = SUCCESS1;
						   string s= Convert4IntToString (TDataint4[0], TDataint4[1], TDataint4[2], TDataint4[3]); 
						   copy_4Int_string_uint8_t (s,data);
						   SendToOutsideCluster (data, PbftNode::m_clusters[index] );
					   }

					 
                    }
                    
                    break;
                   }
                case REPLY:
                 {
					 ////NS_LOG_INFO("node"<< m_id << "    reply "); 
				  if (m_id == (unsigned int) client) { 
				  
				   int Nlimit;
				     if (pbft_Type== RC_PBFT || pbft_Type== PBFT ) Nlimit=N-1;
					   else Nlimit=Ncl; // PBFT_Cluster 
					   
                  //int index = charToInt(msg[2]);
				  int index = TDataint4[2];
                   // NS_LOG_INFO("node"<< m_id << "    reply inin " );
                    tx[index].reply_vote++;
                    
                    if (tx[index].reply_vote>0 && tx[index].reply_vote > (Nlimit / 2)) {// N-1 sans le client --------
                        //data[0] = 20; // any thing ....
                       //data[1] = msg[1];//intToChar(v); //data[2] =  msg[2];     //intToChar(n);  //       //data[3] = SUCCESS1;       // n
						
						TDataint4[0] = 20; TDataint4[3] = SUCCESS1;
						string s= Convert4IntToString (TDataint4[0], TDataint4[1], TDataint4[2], TDataint4[3]); 
						copy_4Int_string_uint8_t (s,data);
						
                        // Send(data);
                        tx[index].reply_vote = -1000;

                        //NS_LOG_INFO("index="<<index);
						Tended[index]=1;
                        
                        
						//float Tr=  Simulator::Now ().GetSeconds ()- tx[index].Tstart; // ajout sof ++
						float Tr=  Simulator::Now ().GetSeconds ()-  TxStart[index]; // ajout sof ++
						//NS_LOG_INFO("node"<< m_id << " now " << Simulator::Now ().GetSeconds ()<<"index="<<index<<" Tstart="<<tx[index].Tstart<< "\n");
                       // NS_LOG_INFO("=== Client node "<< m_id << " msg[2]="<<charToInt(msg[2])<<" in view " << v << "  ---finish--- " << n << "th times-- Now Time ="<< Simulator::Now ().GetSeconds ()<<  "s, Response Time= " << Tr << "s "  );//<< "  value is " << values[block_num]<< "\n\n");
                      //  NS_LOG_INFO("=== Client node "<< m_id << " msg[2]="<<TDataint4[2]<<" in view " << v << "  ---finish--- " << n << "th times-- Now Time ="<< Simulator::Now ().GetSeconds ()<<  "s, Response Time= " << Tr << "s "  );
						NS_LOG_INFO("\n=== Client node "<< m_id << " in view " << v << "  ---finish--- " << TDataint4[2] << "th times-- Now Time ="<< Simulator::Now ().GetSeconds ()<<  "s, Response Time= " << Tr << "s "  );
						tupleDouble tup; tup.t= Simulator::Now ().GetSeconds (); tup.tr=Tr;
						list_reponse_Times.push_back (tup);
						float avg= CalculateAvg(list_reponse_Times);
						float Min= CalculateMin(list_reponse_Times);
						float Max= CalculateMax(list_reponse_Times);
						NS_LOG_INFO("=== Client node "<< m_id << " Response Time (Average)="<<avg<< " (Min)="<<Min<< " (Max)="<<Max<<" \n");			
						
				  //      block_num++;
                        // n = n + 1;
                    }
				  }
                    
                    break;
				
				}
				
				case REPLY_OtherNodes:           
                  {   
                    if ( m_id == (unsigned int) client) break; 
					
                   // int index = charToInt(msg[2]);
					int index = TDataint4[2];
					//NS_LOG_INFO(" node"<< m_id << " Send COMMIT message  n="<<charToInt(msg[2])<<" ");
					
                    // NS_LOG_INFO("node"<< m_id << " commit " << tx[index].val);
                    tx[index].commit_Other_vote++;
                    
                    if (tx[index].commit_Other_vote >0 && tx[index].commit_Other_vote > 2 * (Ncl) / 3) {
                        //data[0] = intToChar(REPLY);    //data[1] = msg[1];//intToChar(v);  //data[2] =  msg[2];    //intToChar(n);   //  //data[3] = SUCCESS1;       // n
						TDataint4[0] = REPLY; TDataint4[3] = SUCCESS1;
						string s= Convert4IntToString (TDataint4[0], TDataint4[1], TDataint4[2], TDataint4[3]); 
						copy_4Int_string_uint8_t (s,data);
						
                        // Send(data);
                        tx[index].commit_Other_vote = -1000;

                       // NS_LOG_INFO("node "<< m_id << " in view " << v << " finish n=" << n << "th block index="<<index<<" :submit, at time " << Simulator::Now ().GetSeconds () << "s,"); //value is " << values[block_num] << "\n");
                       
                        NS_LOG_INFO(" Outside : node "<< m_id << " in view " << v << " finish " << index << "th block  :submit, at time " << Simulator::Now ().GetSeconds () << "s,");
						//block_num++;
                        // n = n + 1;
					   SendTo(data,client);   
                    }
					break;
				}

                case VIEW_CHANGE:
                {
                    NS_LOG_INFO(" in view-change!!!!!!!!!!!!!!!!!\n");
					
					break;
                } 

                default:
                {
                    NS_LOG_INFO("Wrong msg");
                    break;
                }
            }
			free (data);  // ++
        }
        socket->GetSockName (localAddress);
    }
}


void
PbftNode:: ViewChangeTaitements (int nn)
{
	 //uint8_t data[100];
	 
	 int index = nn;
	 
	 ViewChangeNbre++;
	 uint8_t data[size];
	 
	 tx[index].prepare_vote = 0;
	 tx[index].commit_vote = 0;
	 tx[index].commit_Other_vote = 0;
	 tx[index].reply_vote=0;
	 
						
	int leader = PbftNode::m_clusters_leaders[nn]; // cluster 
	//int leader = PbftNode::m_clusters_leaders[1];
						
    NS_LOG_INFO("view-change   leader= " << leader << " view= " << v);
	
    if (v>limit_view_change) {
		                      display_stat();
		                      NS_LOG_INFO("Limits of view of change reached!!!  --- time="<<Simulator::Now ().GetSeconds ()<<" seconds \n"); 
	                          exit(-4);}             				
	
	string s= Convert4IntToString (REQUEST, v, nn, leader); 
	copy_4Int_string_uint8_t (s,data);
								
	//data[0] = intToChar(REQUEST); 
    //data[1] = intToChar(v);         // v
    //data[2] = intToChar(nn);  //intToChar(n);          // n
    //data[3] =intToChar(leader);           // value

	  
	//int index = charToInt(data[2]);
	
                      
    NS_LOG_INFO("re-start consensus --- time="<<Simulator::Now ().GetSeconds ()<<" n="<<nn<<" \n");
					   
	Simulator::Schedule (Seconds(CV_timeout), &PbftNode::TimeoutProc, this,index); // ajout sof ++
	Tended[n]=0;
		   
	//if (Faulty[m_id]!='1') 
		SendTo(data,leader); 	
}

void
PbftNode::viewChange (int nn)
{
	 //int leader = PbftNode::m_clusters_leaders[nn];
	
	/*
    leader = leader + 1;
    if (leader >N ) leader =1;
	*/
	if (pbft_Type==PBFT) { leader = leader + 1;
                           if (leader >N ) leader =1; }
    else if (pbft_Type==PBFT_Cluster) random_cluster(N,Ncl,nn);
	else if (pbft_Type==RC_PBFT) random_cluster(N,Ncl,nn); // may be changed .............
	
	//PbftNode::m_clusters_leaders[nn]=leader; //// cluster 
	
	// PbftNode::m_clusters_leaders[1]=leader;
    //leader = (leader + 1) % (N-1)+1;
    v += 1;
 
	NS_LOG_INFO("+++ view-change, " << leader << " view " << v);
	printCluster(PbftNode::m_clusters[nn]); 
	//for (int i=0;i<8;i++)
		//NS_LOG_INFO("+++" << PbftNode::m_clusters_leaders[i]<<" ");

  ViewChangeTaitements (nn);
  
  //Send(data);  
}

std::string 
PbftNode::getPacketContent(Ptr<Packet> packet, Address from) 
{ 
    char *packetInfo = new char[packet->GetSize () + 1];
    std::ostringstream totalStream;
    packet->CopyData (reinterpret_cast<uint8_t*>(packetInfo), packet->GetSize ());
    packetInfo[packet->GetSize ()] = '\0'; // ensure that it is null terminated to avoid bugs
    /**
     * Add the buffered data to complete the packet
     */
    totalStream << m_bufferedData[from] << packetInfo; 
    std::string totalReceivedData(totalStream.str());

    return totalReceivedData;
}  

void 
SendPacket(Ptr<Socket> socketClient,Ptr<Packet> p) {
    socketClient->Send(p);
}

// send
void 
PbftNode::Send(uint8_t data[], Address from)
{
    Ptr<Packet> p;
   // p = Create<Packet> (data, 4);
    p = Create<Packet> (data, size);
    //NS_LOG_INFO("packet: " << p);
    
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  

    Ptr<Socket> socketClient;
    if (!m_peersSockets[InetSocketAddress::ConvertFrom(from).GetIpv4 ()]) {
        socketClient = Socket::CreateSocket (GetNode (), tid);
        socketClient->Connect (InetSocketAddress(InetSocketAddress::ConvertFrom(from).GetIpv4 (), 7071));
        m_peersSockets[InetSocketAddress::ConvertFrom(from).GetIpv4 ()] = socketClient;
    }
    socketClient = m_peersSockets[InetSocketAddress::ConvertFrom(from).GetIpv4 ()];
    Simulator::Schedule(Seconds(getRandomDelay()), SendPacket, socketClient, p);
}

// 
void 
PbftNode::Send (uint8_t data[])
{   
  Ptr<Packet> p;
  p = Create<Packet> (data, size);
  
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();

  while(iter != m_peersAddresses.end()) {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    
    Ptr<Socket> socketClient = m_peersSockets[*iter];
    double delay = getRandomDelay();
    Simulator::Schedule(Seconds(delay), SendPacket, socketClient, p);
    iter++;
  }
}

// ////////////////
void 
PbftNode::SendTo (uint8_t data[], int id_Node)
{   
   if (id_Node==(int)GetNode ()->GetId ()) return; // do not send to it self 
  
  Ptr<Packet> p;
  p = Create<Packet> (data, size);
  
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();
  int i=0,j=GetNode ()->GetId ();
  while(iter != m_peersAddresses.end()) {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    
 // if ( (i<j && id_Node==i) || (i>j && id_Node==i-1) )
	  if (i==j) i++;
	  if (id_Node==i)
	 {Ptr<Socket> socketClient = m_peersSockets[*iter];
      double delay = getRandomDelay();
      Simulator::Schedule(Seconds(delay), SendPacket, socketClient, p);
	 // NS_LOG_INFO("sendto "<<i<<"\n");
	 }
    iter++;
	i++;
  }
}

///////////////
void 
PbftNode::SendTo (Ptr<Packet> p, int id_Node)
{   
   if (id_Node==((int)GetNode ()->GetId ())) return; // do not send to it self 

  
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();
  int i=0,j=GetNode ()->GetId ();
  while(iter != m_peersAddresses.end()) {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    
	if (i==j) i++;
	if (id_Node==i)
	//if ( (i<j && id_Node==i) || (i>j && id_Node==i-1) )
	 {Ptr<Socket> socketClient = m_peersSockets[*iter];
      double delay = getRandomDelay();
      Simulator::Schedule(Seconds(delay), SendPacket, socketClient, p);
	  // NS_LOG_INFO("sendto "<<i<<"\n");
	   //exit(2);
	 }
    iter++;
	i++;
  }
}


//////////////////////////////
void 
PbftNode::SendToAllCluster (Ptr<Packet> p, std::vector<int> cluster )
{
	//std::map<int, std::vector<int>>    ns3::PbftNode::m_clusters;
	for (auto const &i: cluster) 
	   SendTo (p, i);
	
}


void 
PbftNode::SendToAllCluster (uint8_t data[], std::vector<int> cluster )
{
	//std::map<int, std::vector<int>>    ns3::PbftNode::m_clusters;
	for (auto const &i: cluster) 
	   SendTo (data, i);
	
}

void 
PbftNode::SendToOutsideCluster (uint8_t data[], std::vector<int> cluster )
{
	//std::map<int, std::vector<int>>    ns3::PbftNode::m_clusters;
	for (int i=1;i<N;i++)
	{ int incluster=0;
	  for (auto const &j: cluster)
          if (i==j) incluster=1;
	  
	  if (incluster==0) SendTo (data, i);
    }	
}



void PbftNode::TimeoutProc(int nn){
	//NS_LOG_INFO("proc index n="<<nn);
	 if (Tended[nn]==0 ) { 
	                      NS_LOG_INFO("Timeout ++++++ \n");
						  //n--; ////
                          viewChange (nn);
						  
	 }						  
}


// send block/transaction 
void 
PbftNode::SendBlock (void)
{   
  // NS_LOG_INFO(" time now: " << Simulator::Now ().GetSeconds () << " s");
  Ptr<Packet> p;
  
 

  //std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();
  
  if (m_id == (unsigned int) client) { // client noeud 0
			int leader;
			if (pbft_Type==PBFT) { leader=1; // start always by leader =1 
			                       random_cluster(N,N-1,n); }
			if (pbft_Type==PBFT_Cluster) {random_cluster(N,Ncl,n);
										  leader = PbftNode::m_clusters_leaders[n];}
			else if (pbft_Type==RC_PBFT) {random_cluster(N,Ncl,n); // may be changed .............  
			                             leader = PbftNode::m_clusters_leaders[n];}
										
              // if PBFT no change ....
			  
			//random_cluster(N,Ncl,n); /////
  
			//int leader = PbftNode::m_clusters_leaders[n];
  
			//  int leader = PbftNode::m_clusters_leaders[1];
			//uint8_t * data = generateTX(size, leader);
  
            uint8_t * data = generateTX(100, leader);
			p = Create<Packet> (data, size);
   
			TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  
	    //tx[n].Tstart=Simulator::Now ().GetSeconds (); // ajout sof ++
		NS_LOG_INFO("\n\n------------------ new Block ------Size="<<size<<" Bytes ---------");
		
        NS_LOG_INFO(" Client node"<< m_id << " ---start block n="<<n<<" send to leader "<<leader<< " , at time " <<Simulator::Now ().GetSeconds () << "s");
	
	    //NS_LOG_INFO("==== Leader="<<leader);
	   NS_LOG_INFO(" Leader node"<< leader<< " ");
	   printCluster(PbftNode::m_clusters[n]); 
	   NS_LOG_INFO("");
	
	   TxStart[n]= Simulator::Now ().GetSeconds (); //// pour evaluer le  time out  ++......
	   list_creation_Times.push_back (Simulator::Now ().GetSeconds ());
	   
	  SendTo(p,leader);
	  
       n_round++;
	   
	  Simulator::Schedule (Seconds(CV_timeout), &PbftNode::TimeoutProc, this,n); // ajout sof ++
	   Tended[n]=0;
	   //NS_LOG_INFO("index n="<<n);
	   
	   //n++;
	   n=(n+1)%nMax;
	   v=0;
	   
	   
	  if (Arr_model==constant_period) blockEvent = Simulator::Schedule (Seconds(Block_creation_period), &PbftNode::SendBlock, this);
     else {float tt= next_time_interval (lambda ); // poissonian_arrival
           blockEvent = Simulator::Schedule (Seconds(tt), &PbftNode::SendBlock, this);
	      }   
	}


  
  if (Simulator::Now ().GetSeconds ()>TimeStopEvents) { // secondes 
   // NS_LOG_INFO(" num round "<< n_round << " at time: " << Simulator::Now ().GetSeconds () << "s");
    Simulator::Cancel(blockEvent);
	 display_stat();
	 exit(-2);
	
//  if (m_id == (unsigned int) client) DisplayList(list_reponse_Times);
 
  }
}

} 