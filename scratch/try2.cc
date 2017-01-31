#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-helper.h"
using namespace ns3;
int main (int argc, char *argv[]) {
 bool verbose = true;
 uint32_t nCsma = 3;
 CommandLine cmd;
 cmd.AddValue("nCsma", "Number of ", nCsma);
 cmd.AddValue("verbose", "Log if true", verbose);
 cmd.Parse(argc,argv);
 nCsma = nCsma == 0 ? 1 : nCsma;
// ./waf --run "second --nCsma=3"
 NodeContainer p2pNodes;
 p2pNodes.Create(2);
 NodeContainer csmaNodes;
 csmaNodes.Add(p2pNodes.Get(1));
 csmaNodes.Create(nCsma);
 PointToPointHelper pointToPoint;
 pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
 pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
 NetDeviceContainer p2pDevices;
 p2pDevices = pointToPoint.Install(p2pNodes);
 CsmaHelper csma;
 csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
 csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
 NetDeviceContainer csmaDevices;
 csmaDevices = csma.Install(csmaNodes);
 InternetStackHelper stack;
 stack.Install(p2pNodes.Get(0));
 stack.Install(csmaNodes);
 Ipv4AddressHelper address;
 address.SetBase("10.1.1.0", "255.255.255.0");
 Ipv4InterfaceContainer p2pInterfaces;
 p2pInterfaces = address.Assign(p2pDevices);
 address.SetBase("10.1.2.0", "255.255.255.0");
 Ipv4InterfaceContainer csmaInterfaces;
 csmaInterfaces = address.Assign(csmaDevices);
UdpEchoServerHelper echoServer(9);
 ApplicationContainer serverApps =echoServer.Install(csmaNodes.Get(nCsma));
 serverApps.Start(Seconds(1.0));
 serverApps.Stop(Seconds(10.0));
 UdpEchoClientHelper echoClient(csmaInterfaces.GetAddress(nCsma), 9);
 echoClient.SetAttribute("MaxPackets", UintegerValue(1));
 echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
 echoClient.SetAttribute("PacketSize", UintegerValue(1024));
 ApplicationContainer clientApps =echoClient.Install(p2pNodes.Get(0));
 clientApps.Start(Seconds(2.0));
 clientApps.Stop(Seconds(10.0));
 Ipv4GlobalRoutingHelper::PopulateRoutingTables();
 pointToPoint.EnablePcapAll("second");
 csma.EnablePcap("second", csmaDevices.Get(1), true);
 Simulator::Run();
 Simulator::Destroy();
 return 0;
}
