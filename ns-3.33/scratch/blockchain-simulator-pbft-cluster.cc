#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BlockchainSimulator");



void display_next_connected_addresses (NetworkHelper networkHelper, NodeContainer nodes, int N)
{
	 for (int i = 0; i < N; i++) {
    std::cout <<"\n Node" << nodes.Get (i)->GetId () << " Adresses next hopes (destinations) : ";
	//std::map<uint32_t, std::vector<Ipv4Address>>
	std::vector<Ipv4Address>::iterator iter = networkHelper.m_nodesConnectionsIps[i].begin();
	int j=0;
	 while(iter != networkHelper.m_nodesConnectionsIps[i].end()) {
	 if (i==j)	 std::cout <<"-.-.-.- ";
	 std::cout <<*iter<<" ";
	 
	 iter++;
	 j++;
	 }
	 
   }
   std::cout <<"\n" ;
   
}

void display_local_interfaces_addresses (NetworkHelper networkHelper, NodeContainer nodes, int N)
{
	 for (int i = 0; i < N; i++) {
    std::cout <<"\n Node" << nodes.Get (i)->GetId () << " Adresses local Interfaces : ";
	
	std::vector<Ipv4Address>::iterator iter = networkHelper.m_nodesLocalInterfacesIps[i].begin();
	int j=0;
	 while(iter != networkHelper.m_nodesLocalInterfacesIps[i].end()) {
	 if (i==j)	 std::cout <<"-.-.-.- ";
	 std::cout <<*iter<<" ";
	
	 iter++;
	 j++;
	 }
	 
   }
   std::cout <<"\n" ;
   
}


void startSimulator (int N)
{
  NodeContainer nodes;
  nodes.Create (N);

  NetworkHelper networkHelper (N);
  //pointToPint
  NetDeviceContainer devices;
  PointToPointHelper pointToPoint;

  //  24Mbps, 3Mbps
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("30Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("3ms"));
  //uint32_t nNodes = nodes.GetN ();

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("1.0.0.0", "255.255.255.0");

  
  for (int i = 0; i < N; i++) {
	  
      for (int j = 0; j < N && j != i; j++) {
          Ipv4InterfaceContainer interface;
          Ptr<Node> p1 = nodes.Get (i);
          Ptr<Node> p2 = nodes.Get (j);
          NetDeviceContainer device = pointToPoint.Install(p1, p2);
          
          interface.Add(address.Assign (device.Get(0)));
          interface.Add(address.Assign (device.Get(1)));
		  
		  //networkHelper.m_nodesLocalInterfacesIps   add local ip interfaces addresses
          networkHelper.m_nodesLocalInterfacesIps[i].push_back(interface.GetAddress(0));// ajour
          networkHelper.m_nodesLocalInterfacesIps[j].push_back(interface.GetAddress(1));// ajout
          
		  //address of next connected  interfaces
          networkHelper.m_nodesConnectionsIps[i].push_back(interface.GetAddress(1));
          networkHelper.m_nodesConnectionsIps[j].push_back(interface.GetAddress(0));

          
         address.NewNetwork();
      }
  }
  
  ApplicationContainer nodeApp = networkHelper.Install (nodes);
  
  display_local_interfaces_addresses (networkHelper, nodes, N);
  
  display_next_connected_addresses (networkHelper, nodes, N);
  
   
  //pointToPoint.EnablePcapAll ("./pcap/filepcap");// il faut creer le repertoire pcap dans ~/ns-allinone-3.33/ns-3.33/
  
  nodeApp.Start (Seconds (0.0));
  nodeApp.Stop (Seconds (10.0));

  Simulator::Run ();
  Simulator::Destroy ();
}


int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  // numbers of nodes in network
  int N = 20;
  
  Time::SetResolution (Time::NS);

  // 1.need changed to a specific protocol class
  LogComponentEnable ("PbftNode", LOG_LEVEL_INFO);

  // start the simulator
  startSimulator(N);

  return 0;
}