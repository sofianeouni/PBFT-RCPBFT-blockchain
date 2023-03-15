#ifndef PBFT_NODE_H
#define PBFT_NODE_H

#include <algorithm>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include <map>

#include <iostream>
#include <sstream>

using namespace std;

namespace ns3 {

class Address;
class Socket;
class Packet;

class PbftNode : public Application 
{
  public:
    static TypeId GetTypeId (void);

    void SetPeersAddresses (const std::vector<Ipv4Address> &peers);         

    PbftNode (void);

    virtual ~PbftNode (void);

    uint32_t        m_id;                               // node id
    Ptr<Socket>     m_socket;                           // socket
    Ptr<Socket>     m_socketClient;                     // socket
    std::map<Ipv4Address, Ptr<Socket>>      m_peersSockets;            // socket
    std::map<Address, std::string>          m_bufferedData;            // map holding the buffered data from previous handleRead events
    
    Address         m_local;                            
	
	 std::vector<Ipv4Address>  m_localAddresses;        // ajout
    std::vector<Ipv4Address>  m_peersAddresses;        
	
	static std::map<int, std::vector<int>>    m_clusters; // in map the key is the transaction number list of clusters (cluster is  a list of nodes) 
	static std::map<int, int>                 m_clusters_leaders; // leader for cluster , the map key is the transaction or th block 
	

    int             N;                                  
    // int             v;                                  
    // int             n;                                  
    std::vector<int>  values;                           
    int             value;                              
    int             leader;                             // leader number
    int             is_leader;                          // leader
    int             client=0;                        // le client est le noeud 0
    // int             round;                              
    int             block_num;                             
    EventId         blockEvent ;                         
    struct TX {
        int v;
        int val;
        int prepare_vote;
        int commit_vote;
		int reply_vote;
		int commit_Other_vote;
		float Tstart; // ajout sof ++
		float TstartT; // ajout sof ++ pour le timeout 
    };
    TX tx[100000];


    void random_cluster(int N,int Ncl, int nBlock);
	void display_stat();
	
    // Application
    virtual void StartApplication (void);    
    virtual void StopApplication (void); 

    //
    void HandleRead (Ptr<Socket> socket);

    //
    std::string getPacketContent(Ptr<Packet> packet, Address from); 

    // 
    void Send (uint8_t data[]);

    // 
    void Send(uint8_t data[], Address from);
	
	// ajout
	void SendTo (uint8_t data[], int id_Node);
	
	void SendTo (Ptr<Packet> p, int id_Node);
	void SendToAllCluster (Ptr<Packet> p, std::vector<int> cluster );
	void SendToAllCluster (uint8_t data[], std::vector<int> cluster );
	void SendToOutsideCluster (uint8_t data[], std::vector<int> cluster );
	
	void TimeoutProc(int nn);

    //
    void SendBlock(void);
	
	void ViewChangeTaitements (int nn);
    void viewChange(int nn);
	
	void SendBlock1 (void);
};

enum Message
{
    REQUEST,           // 0     
    PRE_PREPARE,       // 1      
    PREPARE,           // 2      
    COMMIT,            // 3      
    PRE_PREPARE_RES,   // 4       
    PREPARE_RES,       // 5      
    COMMIT_RES,        // 6       
    REPLY,             // 7              
    VIEW_CHANGE ,       // 8       view_change
	REPLY_OtherNodes   // 9    notre approche RC-PBFT 
};

enum State
{
    SUCCESS1,            // 0      成功
    FAILED,             // 1      失败
};
}
#endif
