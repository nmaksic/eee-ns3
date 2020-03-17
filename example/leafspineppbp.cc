/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authon: Natasa Maksic, maksicn@etf.rs
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-coalescing-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"


#define FLOWS_START 3.0
#define FLOWS_STOP 3.1
#define DATA_RATE "10Gbps"
#define DATA_RATE_SERVER "5Gbps"

using namespace ns3;


NodeContainer switches;
NodeContainer network;
NetDeviceContainer devices;
NodeContainer devicenodes;
NetDeviceContainer switchdevices;
NetDeviceContainer serverdevices;
NetDeviceContainer switchserverdevices;
NodeContainer servers;
Ipv4InterfaceContainer serverinterfaces;

char networkadr[100];

unsigned long packets = 0;

NS_LOG_COMPONENT_DEFINE ("LeafSpinePpbp");

bool verbose = false;


void TxTrace(std::string context, Ptr<const Packet> packet)
{
  if(verbose)
  {
    NS_LOG_UNCOND("Packet transmitted by "
    << context << " Time: "
    << Simulator::Now().GetSeconds());
  }
}

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      packets++;
      if(verbose)
      {   
        NS_LOG_UNCOND("Received one packet at "
        << Simulator::Now().GetSeconds());
      }
    }
}

void addudpserver(int server1, NodeContainer &servers, int socketPort) {

  Ptr<Ipv4> ipv4 = servers.Get(server1)->GetObject<Ipv4>();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  Ipv4Address addri = iaddr.GetLocal ();

  Ptr<Socket> recvSink = Socket::CreateSocket (servers.Get (server1), UdpSocketFactory::GetTypeId ());
  InetSocketAddress local = InetSocketAddress (addri, socketPort);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

}


void addudpclient(int server1, int server2, NodeContainer &servers, int socketPort) {

  Ptr<Ipv4> ipv4 = servers.Get(server1)->GetObject<Ipv4>();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  Ipv4Address addri = iaddr.GetLocal ();

  PPBPHelper ppbp = PPBPHelper ("ns3::UdpSocketFactory", InetSocketAddress (addri,socketPort));

  ppbp.SetAttribute ("BurstIntensity", DataRateValue (DataRate ("10Mbps")));  
  ppbp.SetAttribute ("PacketSize", UintegerValue (1000));                     
  ppbp.SetAttribute ("MeanBurstArrivals", DoubleValue (1000));                
  ppbp.SetAttribute ("MeanBurstTimeLength", DoubleValue (0.001));             
  ppbp.SetAttribute ("H", DoubleValue (0.7));                                 

   
  ApplicationContainer apps = ppbp.Install (servers.Get (server2));
  apps.Start (Seconds (FLOWS_START));
  apps.Stop (Seconds (FLOWS_STOP));

}



char *getnextnetwork() {

	static int i = 1;
	static int j = 0;
	
	j++;
	if (j==255) {
		if (i==255) 
			printf("addresses exausted\n");
		i++;
		j=1;	
	}

	sprintf(networkadr, "10.%i.%i.0", i, j);
	
	return networkadr;
}

void connect(NodeContainer *pplink, unsigned int *address1, unsigned int *address2, unsigned int *interface1, unsigned int *interface2) {

  PointToPointCoalescingHelper pointToPointCoalescing;
  pointToPointCoalescing.SetDeviceAttribute ("DataRate", StringValue (DATA_RATE));
  pointToPointCoalescing.SetChannelAttribute ("Delay", StringValue ("30us"));

   

	NetDeviceContainer p2pDevices;
	p2pDevices = pointToPointCoalescing.Install (*pplink);

	Ipv4AddressHelper address;
	char *adr = getnextnetwork();
	address.SetBase (adr, "255.255.255.0");
	Ipv4InterfaceContainer p2pInterfaces;
	p2pInterfaces = address.Assign (p2pDevices);

	devices.Add(p2pDevices);
	devicenodes.Add(*pplink);
	switchdevices.Add(p2pDevices);

	Ipv4Address addr1 = p2pInterfaces.GetAddress(0, 0);
	Ipv4Address addr2 = p2pInterfaces.GetAddress(1, 0);

	*address1 = addr1.Get();
	*address2 = addr2.Get();
	
	Ptr<Ipv4> ipv4 = pplink->Get(0)->GetObject<Ipv4>();
	*interface1 = ipv4->GetInterfaceForAddress(addr1);
	ipv4 = pplink->Get(1)->GetObject<Ipv4>();
	*interface2 = ipv4->GetInterfaceForAddress(addr2);

   for (int ifc = 0; ifc < 2; ifc++) {
	   p2pDevices.Get(ifc)->SetAttribute ("EeeCoalescingTimeout", DoubleValue (800)); 
	   p2pDevices.Get(ifc)->SetAttribute ("EeeByteLimit", DoubleValue (24000)); 
	   p2pDevices.Get(ifc)->SetAttribute ("EeeSleepTime", DoubleValue (2.88)); 
	   p2pDevices.Get(ifc)->SetAttribute ("EeeWakeUpTime", DoubleValue (4.48)); 
   }
	
}


void leafspine(int switchcount) {

	switches.Create(switchcount);
	network.Add(switches);


  InternetStackHelper internet;
  internet.Install (switches);

	for ( int i = 0; i < switchcount/2; i++) {
		for ( int j = switchcount/2; j < switchcount; j++) {

				NodeContainer newlink;

				newlink.Add(switches.Get(i));
				newlink.Add(switches.Get(j));	
				
				unsigned int address1;
				unsigned int address2;
				unsigned int interface1;
				unsigned int interface2;
				connect(&newlink, &address1, &address2, &interface1, &interface2);

				std::cout << i << "-> : " << std::hex << address1 << ":" <<  interface1 << " " << address2 << ":" <<  interface2 << std::dec << std::endl;

            std::cout << "add link " << (switches.Get(i))->GetId() << " " << (switches.Get(j))->GetId() << std::endl;

		}
	}
		
}

void addservers(int swservers) {

	int sw = switches.GetN();

  InternetStackHelper internetNodes;
  internetNodes.Install (servers);

	for (int i = 0; i < sw/2; i++) {

		for (int j=0; j<swservers; j++) {

			NodeContainer p2pNodes;

			p2pNodes.Add (switches.Get(i));
			p2pNodes.Create (1);			

			internetNodes.Install(p2pNodes.Get(1));
			
			PointToPointCoalescingHelper pointToPoint;
			pointToPoint.SetDeviceAttribute ("DataRate", StringValue (DATA_RATE_SERVER));
			pointToPoint.SetChannelAttribute ("Delay", StringValue ("30us"));

			NetDeviceContainer p2pDevices;

			p2pDevices = pointToPoint.Install (p2pNodes);

			Ipv4AddressHelper address;
			char *adr = getnextnetwork();
			address.SetBase (adr, "255.255.255.0");
			Ipv4InterfaceContainer p2pInterfaces;
			p2pInterfaces = address.Assign (p2pDevices);

			servers.Add(p2pNodes.Get(1));
			serverinterfaces.Add(p2pInterfaces.Get(1));
			network.Add(p2pNodes.Get(1));
	
			devices.Add(p2pDevices);
			devicenodes.Add(p2pNodes);
			serverdevices.Add(p2pDevices.Get(1));

         switchserverdevices.Add(p2pDevices.Get(0));

         for (int ifc = 0; ifc < 2; ifc++) {
            p2pDevices.Get(ifc)->SetAttribute ("EeeCoalescingTimeout", DoubleValue (800)); 
            p2pDevices.Get(ifc)->SetAttribute ("EeeByteLimit", DoubleValue (15000)); 
            p2pDevices.Get(ifc)->SetAttribute ("EeeSleepTime", DoubleValue (5)); 
            p2pDevices.Get(ifc)->SetAttribute ("EeeWakeUpTime", DoubleValue (8)); 
         }

			
			std::cout << "add server " << (p2pNodes.Get(0))->GetId() << " " << (p2pNodes.Get(1))->GetId() << std::endl;

		}

	}

}



int
main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));

  int switchcount = 8;
  int serversperswitch = 16;
  leafspine(switchcount);
  addservers(serversperswitch);

   // add flows
   for (int i = 0; i < 4; i++) 
      for (int j = 0; j < 16; j++) {
         addudpserver(i*16+j, servers, 9001);
         for (int cl = 0; cl < 4; cl++)
            if (cl != i) {
                for (int a = 0; a < 3; a++) 
               addudpclient(i*16+j, cl*16+j, servers, 9001);
            }
      }

  // add routes
  Ptr<Ipv4StaticRouting> staticRouting;
  
  for (int i = 0; i < switchcount/2; i++) 
      for (int j= 0; j < serversperswitch; j++) {
     staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (servers.Get(i*serversperswitch+j)->GetObject<Ipv4> ()->GetRoutingProtocol ());
     
     std::stringstream sgw;
     sgw << "10.1." << (switchcount + i*serversperswitch+j) << ".1";
     staticRouting->SetDefaultRoute (Ipv4Address (sgw.str().c_str()), 1 );
     
      }
   Ptr<Ipv4GlobalRouting> globalRouting;
   for (int i = 0; i < switchcount/2; i++) {
      globalRouting = Ipv4RoutingHelper::GetRouting <Ipv4GlobalRouting> (switches.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
      for (int j = 0; j < switchcount/2; j++) {
         if (i != j)
            for (int s = 0; s < serversperswitch; s++) {
               std::stringstream sdstnet;
               sdstnet << "10.1." << (switchcount + j*serversperswitch+s+1) << ".0";
	            Ipv4Address network(sdstnet.str().c_str());
               std::string netmask("255.255.255.0");
	            Ipv4Mask networkmask(netmask.c_str());			
               for (int ifc = 1; ifc <= switchcount/2; ifc++) {
   	            globalRouting->AddNetworkRouteTo(network, networkmask, ifc);
               }
            }
        }
   }

   for (int i = switchcount/2; i < switchcount; i++) {
      globalRouting = Ipv4RoutingHelper::GetRouting <Ipv4GlobalRouting> (switches.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
      for (int ifc = 1; ifc <= switchcount/2; ifc++)
            for (int s = 0; s < serversperswitch; s++) {
               std::stringstream sdstnet;
               sdstnet << "10.1." << (switchcount + (ifc-1)*serversperswitch+s+1) << ".0";
	            Ipv4Address network(sdstnet.str().c_str());
               std::string netmask("255.255.255.0");
	            Ipv4Mask networkmask(netmask.c_str());			
   	         globalRouting->AddNetworkRouteTo(network, networkmask, ifc);
            }
  }



  Simulator::Stop (Seconds (10.0));


  Simulator::Run ();
 
  
  // Write measurements data
  for (unsigned int i = 0; i < switchdevices.GetN(); i++) {
     Ptr<PointToPointCoalescingNetDevice> dev = DynamicCast<PointToPointCoalescingNetDevice> (switchdevices.Get(i));
     dev->WriteMeasurementsData (DATA_RATE);
     
  }

  for (unsigned int i = 0; i < serverdevices.GetN(); i++) {
     Ptr<PointToPointCoalescingNetDevice> dev = DynamicCast<PointToPointCoalescingNetDevice> (serverdevices.Get(i));
     dev->WriteMeasurementsData (DATA_RATE_SERVER);
      
  }

  for (unsigned int i = 0; i < switchserverdevices.GetN(); i++) {
     Ptr<PointToPointCoalescingNetDevice> dev = DynamicCast<PointToPointCoalescingNetDevice> (switchserverdevices.Get(i));
     dev->WriteMeasurementsData (DATA_RATE_SERVER);
      
  }

  Simulator::Destroy ();

  std::cout << "total packets " << packets << std::endl;

  return 0;
}
