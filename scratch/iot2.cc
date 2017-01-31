#include "ns3/core-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/mpi-interface.h"
#define MPI_TEST

#ifdef NS3_MPI
#include <mpi.h>
#endif

using namespace ns3;

int main(int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  /* Configuration. */
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  /* Build nodes. */
  NodeContainer term_0;
  term_0.Create (1);
  NodeContainer term_1;
  term_1.Create (1);
  NodeContainer term_2;
  term_2.Create (1);
  NodeContainer term_3;
  term_3.Create (1);
  NodeContainer term_4;
  term_4.Create (1);
  NodeContainer ap_0;
  ap_0.Create (1);
  NodeContainer term_5;
  term_5.Create (1);
  NodeContainer term_6;
  term_6.Create (1);
  NodeContainer term_7;
  term_7.Create (1);
  NodeContainer term_8;
  term_8.Create (1);
  NodeContainer ap_1;
  ap_1.Create (1);
  NodeContainer term_9;
  term_9.Create (1);
  NodeContainer term_10;
  term_10.Create (1);
  NodeContainer term_11;
  term_11.Create (1);
  NodeContainer term_12;
  term_12.Create (1);
  NodeContainer ap_2;
  ap_2.Create (1);
  NodeContainer term_13;
  term_13.Create (1);
  NodeContainer ap_3;
  ap_3.Create (1);
  NodeContainer term_16;
  term_16.Create (1);
  NodeContainer term_18;
  term_18.Create (1);
  NodeContainer term_19;
  term_19.Create (1);
  NodeContainer term_21;
  term_21.Create (1);
  NodeContainer term_22;
  term_22.Create (1);
  NodeContainer term_23;
  term_23.Create (1);
  NodeContainer term_24;
  term_24.Create (6);

  /* Build link. */
  YansWifiPhyHelper wifiPhy_ap_0 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel_ap_0 = YansWifiChannelHelper::Default ();
  wifiPhy_ap_0.SetChannel (wifiChannel_ap_0.Create ());
  YansWifiPhyHelper wifiPhy_ap_1 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel_ap_1 = YansWifiChannelHelper::Default ();
  wifiPhy_ap_1.SetChannel (wifiChannel_ap_1.Create ());
  YansWifiPhyHelper wifiPhy_ap_2 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel_ap_2 = YansWifiChannelHelper::Default ();
  wifiPhy_ap_2.SetChannel (wifiChannel_ap_2.Create ());
  YansWifiPhyHelper wifiPhy_ap_3 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel_ap_3 = YansWifiChannelHelper::Default ();
  wifiPhy_ap_3.SetChannel (wifiChannel_ap_3.Create ());
  CsmaHelper csma_hub_12;
  csma_hub_12.SetChannelAttribute ("DataRate", DataRateValue (100000000));
  csma_hub_12.SetChannelAttribute ("Delay",  TimeValue (MilliSeconds (10000)));

  /* Build link net device container. */
  NodeContainer all_ap_0;
  NetDeviceContainer ndc_ap_0;
  Ssid ssid_ap_0 = Ssid ("wifi-default-0");
  WifiHelper wifi_ap_0 = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac_ap_0 = NqosWifiMacHelper::Default ();
  wifi_ap_0.SetRemoteStationManager ("ns3::ArfWifiManager");
  wifiMac_ap_0.SetType ("ns3::ApWifiMac",
     "Ssid", SsidValue (ssid_ap_0),
     "BeaconGeneration", BooleanValue (true),
     "BeaconInterval", TimeValue (Seconds (2.5)));
  ndc_ap_0.Add (wifi_ap_0.Install (wifiPhy_ap_0, wifiMac_ap_0, ap_0));
  wifiMac_ap_0.SetType ("ns3::StaWifiMac",
     "Ssid", SsidValue (ssid_ap_0),
     "ActiveProbing", BooleanValue (false));
  ndc_ap_0.Add (wifi_ap_0.Install (wifiPhy_ap_0, wifiMac_ap_0, all_ap_0 ));
  MobilityHelper mobility_ap_0;
  mobility_ap_0.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap_0.Install (ap_0);
  mobility_ap_0.Install(all_ap_0);
  NodeContainer all_ap_1;
  NetDeviceContainer ndc_ap_1;
  Ssid ssid_ap_1 = Ssid ("wifi-default-1");
  WifiHelper wifi_ap_1 = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac_ap_1 = NqosWifiMacHelper::Default ();
  wifi_ap_1.SetRemoteStationManager ("ns3::ArfWifiManager");
  wifiMac_ap_1.SetType ("ns3::ApWifiMac",
     "Ssid", SsidValue (ssid_ap_1),
     "BeaconGeneration", BooleanValue (true),
     "BeaconInterval", TimeValue (Seconds (2.5)));
  ndc_ap_1.Add (wifi_ap_1.Install (wifiPhy_ap_1, wifiMac_ap_1, ap_1));
  wifiMac_ap_1.SetType ("ns3::StaWifiMac",
     "Ssid", SsidValue (ssid_ap_1),
     "ActiveProbing", BooleanValue (false));
  ndc_ap_1.Add (wifi_ap_1.Install (wifiPhy_ap_1, wifiMac_ap_1, all_ap_1 ));
  MobilityHelper mobility_ap_1;
  mobility_ap_1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap_1.Install (ap_1);
  mobility_ap_1.Install(all_ap_1);
  NodeContainer all_ap_2;
  NetDeviceContainer ndc_ap_2;
  Ssid ssid_ap_2 = Ssid ("wifi-default-2");
  WifiHelper wifi_ap_2 = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac_ap_2 = NqosWifiMacHelper::Default ();
  wifi_ap_2.SetRemoteStationManager ("ns3::ArfWifiManager");
  wifiMac_ap_2.SetType ("ns3::ApWifiMac",
     "Ssid", SsidValue (ssid_ap_2),
     "BeaconGeneration", BooleanValue (true),
     "BeaconInterval", TimeValue (Seconds (2.5)));
  ndc_ap_2.Add (wifi_ap_2.Install (wifiPhy_ap_2, wifiMac_ap_2, ap_2));
  wifiMac_ap_2.SetType ("ns3::StaWifiMac",
     "Ssid", SsidValue (ssid_ap_2),
     "ActiveProbing", BooleanValue (false));
  ndc_ap_2.Add (wifi_ap_2.Install (wifiPhy_ap_2, wifiMac_ap_2, all_ap_2 ));
  MobilityHelper mobility_ap_2;
  mobility_ap_2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap_2.Install (ap_2);
  mobility_ap_2.Install(all_ap_2);
  NodeContainer all_ap_3;
  NetDeviceContainer ndc_ap_3;
  Ssid ssid_ap_3 = Ssid ("wifi-default-3");
  WifiHelper wifi_ap_3 = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac_ap_3 = NqosWifiMacHelper::Default ();
  wifi_ap_3.SetRemoteStationManager ("ns3::ArfWifiManager");
  wifiMac_ap_3.SetType ("ns3::ApWifiMac",
     "Ssid", SsidValue (ssid_ap_3),
     "BeaconGeneration", BooleanValue (true),
     "BeaconInterval", TimeValue (Seconds (2.5)));
  ndc_ap_3.Add (wifi_ap_3.Install (wifiPhy_ap_3, wifiMac_ap_3, ap_3));
  wifiMac_ap_3.SetType ("ns3::StaWifiMac",
     "Ssid", SsidValue (ssid_ap_3),
     "ActiveProbing", BooleanValue (false));
  ndc_ap_3.Add (wifi_ap_3.Install (wifiPhy_ap_3, wifiMac_ap_3, all_ap_3 ));
  MobilityHelper mobility_ap_3;
  mobility_ap_3.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap_3.Install (ap_3);
  mobility_ap_3.Install(all_ap_3);
  NodeContainer all_hub_1;
  all_hub_1.Add (term_21);
  all_hub_1.Add (term_16);
  all_hub_1.Add (term_19);
  all_hub_1.Add (term_18);
  all_hub_1.Add (term_23);
  all_hub_1.Add (term_22);
  NetDeviceContainer ndc_hub_1 = csma_hub_12.Install (all_hub_1);

  /* Install the IP stack. */
  InternetStackHelper internetStackH;
  internetStackH.Install (term_0);
  internetStackH.Install (term_1);
  internetStackH.Install (term_2);
  internetStackH.Install (term_3);
  internetStackH.Install (term_4);
  internetStackH.Install (ap_0);
  internetStackH.Install (term_5);
  internetStackH.Install (term_6);
  internetStackH.Install (term_7);
  internetStackH.Install (term_8);
  internetStackH.Install (ap_1);
  internetStackH.Install (term_9);
  internetStackH.Install (term_10);
  internetStackH.Install (term_11);
  internetStackH.Install (term_12);
  internetStackH.Install (ap_2);
  internetStackH.Install (term_13);
  internetStackH.Install (ap_3);
  internetStackH.Install (term_16);
  internetStackH.Install (term_18);
  internetStackH.Install (term_19);
  internetStackH.Install (term_21);
  internetStackH.Install (term_22);
  internetStackH.Install (term_23);
  internetStackH.Install (term_24);

  /* IP assign. */
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_ap_0 = ipv4.Assign (ndc_ap_0);
  ipv4.SetBase ("10.0.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_ap_1 = ipv4.Assign (ndc_ap_1);
  ipv4.SetBase ("10.0.2.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_ap_2 = ipv4.Assign (ndc_ap_2);
  ipv4.SetBase ("10.0.3.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_ap_3 = ipv4.Assign (ndc_ap_3);
  ipv4.SetBase ("10.0.4.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_hub_1 = ipv4.Assign (ndc_hub_1);

  /* Generate Route. */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Generate Application. */

  /* Simulation. */
  /* Pcap output. */
  /* Stop the simulation after x seconds. */
  uint32_t stopTime = 1;
  Simulator::Stop (Seconds (stopTime));
  /* Start and clean simulation. */
  Simulator::Run ();
  Simulator::Destroy ();
}
