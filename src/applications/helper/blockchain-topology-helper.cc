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
 * Author: Josh Pelkey <jpelkey@gatech.edu>
 */

#include "ns3/blockchain-topology-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/csma-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/string.h"
#include "ns3/vector.h"
#include "ns3/log.h"
#include "ns3/ipv6-address-generator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
//#include "ns3/blockchain-constants.h"
#include <algorithm>
#include <fstream>
#include <time.h>
#include <sys/time.h>

static double GetWallTime();
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BlockchainTopologyHelper");

BlockchainTopologyHelper::BlockchainTopologyHelper (uint32_t noCpus, uint32_t totalNoNodes, enum BlockchainRegion *minersRegions,
                                               int minConnectionsPerNode, int maxConnectionsPerNode,
						                      double latencyParetoShapeDivider, uint32_t systemId, std::vector<uint32_t> miners, std::map<uint32_t,std::vector<uint32_t> > gatewayChildMap, std::map<uint32_t, uint32_t> gatewayMinerMap)
  : m_noCpus(noCpus), m_totalNoNodes(totalNoNodes), m_minConnectionsPerNode (minConnectionsPerNode), m_maxConnectionsPerNode (maxConnectionsPerNode),
	m_totalNoLinks (0), m_latencyParetoShapeDivider (latencyParetoShapeDivider),
	m_systemId (systemId), m_minConnectionsPerMiner (10), m_maxConnectionsPerMiner (80),
	m_minerDownloadSpeed (100), m_minerUploadSpeed (100), m_miners (miners) , m_nodeGatewayMap (gatewayChildMap), m_gatewayMinerMap (gatewayMinerMap)
{
  std::vector<uint32_t>     nodes;    //nodes contain the ids of all the nodes
  double                    tStart = GetWallTime();
  double                    tFinish;

  int m_noMiners = miners.size();
  m_minConnectionsPerMiner = m_noMiners*(m_noMiners/2);
  int m_gatewayCount = gatewayChildMap.size();
  m_maxConnectionsPerMiner = m_minConnectionsPerMiner + m_gatewayCount;
  //std::map<uint32_t, std::vector<uint32_t>>  m_nodesConnections;

  srand (1000);

  // Bounds check
  if (m_noMiners > m_totalNoNodes)
  {
    NS_FATAL_ERROR ("The number of miners is larger than the total number of nodes\n");
  }

  if (m_noMiners < 1)
  {
    NS_FATAL_ERROR ("You need at least one miner\n");
  }

  m_blockchainNodesRegion = new uint32_t[m_totalNoNodes];


  /*std::array<double,7> nodesDistributionIntervals {NORTH_AMERICA, EUROPE, SOUTH_AMERICA, ASIA_PACIFIC, AFRICA, AUSTRALIA, OTHER};

      std::array<double,6> nodesDistributionWeights {39.24, 48.79, 2.12, 6.97, 1.06, 1.82};
      m_nodesDistribution = std::piecewise_constant_distribution<double> (nodesDistributionIntervals.begin(), nodesDistributionIntervals.end(), nodesDistributionWeights.begin());

  std::array<double,7> connectionsDistributionIntervals {1, 5, 10, 15, 20, 30, 125};
  for (int i = 0; i < 7; i++)
	connectionsDistributionIntervals[i] -= i;

  std::array<double,6> connectionsDistributionWeights {10, 40, 30, 13, 6, 1};*/

  std::cout << "Building the topology!" << std::endl;
  m_minersRegions = new enum BlockchainRegion[m_noMiners];
  for (int i = 0; i < m_noMiners; i++)
  {
    m_minersRegions[m_miners[i]] = minersRegions[m_miners[i]];
  }

  std::cout << "Creating list of all nodes!" << std::endl;
  /**
   * Create a vector containing all the nodes ids
   */
  for (int i = 1; i <= m_totalNoNodes; i++)
  {
    nodes.push_back(i);
  }

  std::cout << "Forming the miners links!" << std::endl;
  sort(m_miners.begin(), m_miners.end());

  //Interconnect the miners
  for(auto &miner : m_miners)
  {
    for(auto &peer : m_miners)
    {
      if (miner != peer)
        m_nodesConnections[miner].push_back(peer);
	}
  }

    std::cout << "Forming the gateway links!" << std::endl;
  std::map<uint32_t, std::vector<uint32_t> >::iterator gateways_it;

  for(gateways_it=m_nodeGatewayMap.begin();gateways_it!=m_nodeGatewayMap.end();gateways_it++)
  {
    uint32_t gatewayId = gateways_it->first;
    std::vector<uint32_t> gatewayChilds = gateways_it->second;
    //Connect all children to gateway
    for(auto &child : gatewayChilds)
    {
      m_nodesConnections[gatewayId].push_back(child);
      m_nodesConnections[child].push_back(gatewayId);
    }
    uint32_t miner = m_gatewayMinerMap[gatewayId];
    m_nodesConnections[gatewayId].push_back(miner);
    m_nodesConnections[miner].push_back(gatewayId);
  }

  std::cout << "All links formed!" << std::endl;
  tFinish = GetWallTime();
  if (m_systemId == 0)
  {
    std::cout << "The nodes connections were created in " << tFinish - tStart << "s.\n";
    /*std::cout << "The minimum number of connections for each node is " << m_minConnectionsPerNode
              << " and whereas the maximum is " << m_maxConnectionsPerNode << ".\n";*/
  }


  InternetStackHelper stack;

  std::ostringstream latencyStringStream;
  std::ostringstream bandwidthStream;

  PointToPointHelper pointToPoint;
  CsmaHelper csma;
  SixLowPanHelper sixlowpan;
  Ipv6AddressHelper ipv6;

  tStart = GetWallTime();

  std::cout << "Creating " << m_totalNoNodes << " nodes!" << std::endl;
  //Create the blockchain nodes
  for (uint32_t i = 0; i <= m_totalNoNodes; i++)
  {
    NodeContainer currentNode;
    //  currentNode.Create (1, i % m_noCpus);
    currentNode.Create (1);
/* 	if (m_systemId == 0)
      std::cout << "Creating a node with Id = " << i << " and systemId = " << i % m_noCpus << "\n"; */
    m_nodes.push_back (currentNode);
	  //AssignRegion(i);
    AssignInternetSpeeds(i);
  }


  //Print region bandwidths averages
  /*if (m_systemId == 0)
  {
    std::map<uint32_t, std::vector<double>> downloadRegionBandwidths;
    std::map<uint32_t, std::vector<double>> uploadRegionBandwidths;

    for(int i = 0; i < m_totalNoNodes; i++)
    {
      if ( std::find(m_miners.begin(), m_miners.end(), i) == m_miners.end())
      {
        downloadRegionBandwidths[m_blockchainNodesRegion[i]].push_back(m_nodesInternetSpeeds[i].downloadSpeed);
        uploadRegionBandwidths[m_blockchainNodesRegion[i]].push_back(m_nodesInternetSpeeds[i].uploadSpeed);
      }
    }

    for (auto region : downloadRegionBandwidths)
    {
       double average = 0;
       for (auto &speed : region.second)
       {
         average += speed;
	   }

      std::cout << "The download speed for region " << getBlockchainRegion(getBlockchainEnum(region.first)) << " = " << average / region.second.size() << " Mbps\n";
    }

    for (auto region : uploadRegionBandwidths)
    {
       double average = 0;
       for (auto &speed : region.second)
       {
         average += speed;
	   }

      std::cout << "The upload speed for region " << getBlockchainRegion(getBlockchainEnum(region.first)) << " = " << average / region.second.size() << " Mbps\n";
    }
  }*/

  tFinish = GetWallTime();
  if (m_systemId == 0)
    std::cout << "The nodes were created in " << tFinish - tStart << "s.\n";

  tStart = GetWallTime();

  std::cout << "Creating the links between the miner nodes!" << std::endl;
  //Create first the links between miners
  for(auto miner = m_miners.begin(); miner != m_miners.end(); miner++)
  {

    for(std::vector<uint32_t>::const_iterator it = m_nodesConnections[*miner].begin(); it != m_nodesConnections[*miner].begin() + m_miners.size() - 1; it++)
    {
      if ( *it > *miner)	//Do not recreate links
      {
        NetDeviceContainer newDevices;

        m_totalNoLinks++;

		/*double bandwidth = std::min(std::min(m_nodesInternetSpeeds[m_nodes.at (*miner).Get (0)->GetId()].uploadSpeed,
                                    m_nodesInternetSpeeds[m_nodes.at (*miner).Get (0)->GetId()].downloadSpeed),
                                    std::min(m_nodesInternetSpeeds[m_nodes.at (*it).Get (0)->GetId()].uploadSpeed,
                                    m_nodesInternetSpeeds[m_nodes.at (*it).Get (0)->GetId()].downloadSpeed));*/
      double bandwidth = 10.05;
		  bandwidthStream.str("");
      bandwidthStream.clear();
		  bandwidthStream << bandwidth << "Mbps";

      latencyStringStream.str("");
      latencyStringStream.clear();

		/*if (m_latencyParetoShapeDivider > 0)
        {
          Ptr<ParetoRandomVariable> paretoDistribution = CreateObject<ParetoRandomVariable> ();
          paretoDistribution->SetAttribute ("Mean", DoubleValue (m_regionLatencies[m_blockchainNodesRegion[(m_nodes.at (*miner).Get (0))->GetId()]]
                                                                                  [m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]]));
          paretoDistribution->SetAttribute ("Shape", DoubleValue (m_regionLatencies[m_blockchainNodesRegion[(m_nodes.at (*miner).Get (0))->GetId()]]
                                                                                   [m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]] / m_latencyParetoShapeDivider));
          latencyStringStream << paretoDistribution->GetValue() << "ms";
        }
        else
        {
          latencyStringStream << m_regionLatencies[m_blockchainNodesRegion[(m_nodes.at (*miner).Get (0))->GetId()]]
                                                  [m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]] << "ms";
        }*/


		pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidthStream.str()));
		//pointToPoint.SetChannelAttribute ("Delay", StringValue (latencyStringStream.str()));

    newDevices.Add (pointToPoint.Install (m_nodes.at (*miner).Get (0), m_nodes.at (*it).Get (0)));
		m_devices.push_back (newDevices);
/* 		if (m_systemId == 0)
          std::cout << "Creating link " << m_totalNoLinks << " between nodes "
                    << (m_nodes.at (*miner).Get (0))->GetId() << " ("
                    <<  getBlockchainRegion(getBlockchainEnum(m_blockchainNodesRegion[(m_nodes.at (*miner).Get (0))->GetId()]))
                    << ") and node " << (m_nodes.at (*it).Get (0))->GetId() << " ("
                    <<  getBlockchainRegion(getBlockchainEnum(m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]))
                    << ") with latency = " << latencyStringStream.str()
                    << " and bandwidth = " << bandwidthStream.str() << ".\n"; */
      }
    }
  }

  std::cout << "Creating the links between rest of the nodes!" << std::endl;
  for(auto &node : m_nodesConnections)
  {

    for(std::vector<uint32_t>::const_iterator it = node.second.begin(); it != node.second.end(); it++)
    {

      if ( *it > node.first && (std::find(m_miners.begin(), m_miners.end(), *it) == m_miners.end() ||
	       std::find(m_miners.begin(), m_miners.end(), node.first) == m_miners.end()))	//Do not recreate links
      {
        NetDeviceContainer newDevices;

        m_totalNoLinks++;

		/*double bandwidth = std::min(std::min(m_nodesInternetSpeeds[m_nodes.at (node.first).Get (0)->GetId()].uploadSpeed,
                                    m_nodesInternetSpeeds[m_nodes.at (node.first).Get (0)->GetId()].downloadSpeed),
                                    std::min(m_nodesInternetSpeeds[m_nodes.at (*it).Get (0)->GetId()].uploadSpeed,
                                    m_nodesInternetSpeeds[m_nodes.at (*it).Get (0)->GetId()].downloadSpeed));*/
      double bandwidth = 9.05;
  		bandwidthStream.str("");
      bandwidthStream.clear();
  		bandwidthStream << bandwidth << "Mbps";

      latencyStringStream.str("");
      latencyStringStream.clear();

		/*if (m_latencyParetoShapeDivider > 0)
        {
          Ptr<ParetoRandomVariable> paretoDistribution = CreateObject<ParetoRandomVariable> ();
          paretoDistribution->SetAttribute ("Mean", DoubleValue (m_regionLatencies[m_blockchainNodesRegion[(m_nodes.at (node.first).Get (0))->GetId()]]
                                                                                  [m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]]));
          paretoDistribution->SetAttribute ("Shape", DoubleValue (m_regionLatencies[m_blockchainNodesRegion[(m_nodes.at (node.first).Get (0))->GetId()]]
                                                                                   [m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]] / m_latencyParetoShapeDivider));
          latencyStringStream << paretoDistribution->GetValue() << "ms";
        }
        else
        {
        latencyStringStream << m_regionLatencies[m_blockchainNodesRegion[(m_nodes.at (node.first).Get (0))->GetId()]]
                                                [m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]] << "ms";
        }*/

		//pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidthStream.str()));
    csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
		//pointToPoint.SetChannelAttribute ("Delay", StringValue (latencyStringStream.str()));
    NodeContainer csmaNodes (m_nodes.at (node.first).Get (0), m_nodes.at (*it).Get (0));
    NetDeviceContainer csmaDevice = csma.Install (csmaNodes);
    csma.SetDeviceAttribute ("Mtu", UintegerValue (150));
    sixlowpan.SetDeviceAttribute ("ForceEtherType", BooleanValue (true) );
    newDevices.Add (sixlowpan.Install (csmaDevice));
    //newDevices.Add (pointToPoint.Install (m_nodes.at (node.first).Get (0), m_nodes.at (*it).Get (0)));
		m_devices.push_back (newDevices);
/* 		if (m_systemId == 0)
          std::cout << "Creating link " << m_totalNoLinks << " between nodes "
                    << (m_nodes.at (node.first).Get (0))->GetId() << " ("
                    <<  getBlockchainRegion(getBlockchainEnum(m_blockchainNodesRegion[(m_nodes.at (node.first).Get (0))->GetId()]))
                    << ") and node " << (m_nodes.at (*it).Get (0))->GetId() << " ("
                    <<  getBlockchainRegion(getBlockchainEnum(m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]))
                    << ") with latency = " << latencyStringStream.str()
                    << " and bandwidth = " << bandwidthStream.str() << ".\n"; */
      }
    }
  }

  tFinish = GetWallTime();

  std::cout << "Created all the links!" << std::endl;
  if (m_systemId == 0)
    std::cout << "The total number of links is " << m_totalNoLinks << " (" << tFinish - tStart << "s).\n";
}

BlockchainTopologyHelper::~BlockchainTopologyHelper ()
{
  delete[] m_blockchainNodesRegion;
  delete[] m_minersRegions;
}

void
BlockchainTopologyHelper::InstallStack (InternetStackHelper stack)
{
  double tStart = GetWallTime();
  double tFinish;

  std::cout << "Installing the internet stack!" << std::endl;
  for (uint32_t i = 0; i < m_nodes.size (); ++i)
    {
      NodeContainer currentNode = m_nodes[i];
      for (uint32_t j = 0; j < currentNode.GetN (); ++j)
        {
          stack.Install (currentNode.Get (j));
        }
    }

  tFinish = GetWallTime();
  if (m_systemId == 0)
    std::cout << "Internet stack installed in " << tFinish - tStart << "s.\n";
}

void
BlockchainTopologyHelper::AssignIpv4Addresses (Ipv4AddressHelperCustom ip)
{
  double tStart = GetWallTime();
  double tFinish;

  // Assign addresses to all devices in the network.
  // These devices are stored in a vector.
  for (uint32_t i = 0; i < m_devices.size (); ++i)
  {
    Ipv4InterfaceContainer newInterfaces;
    NetDeviceContainer currentContainer = m_devices[i];

    newInterfaces.Add (ip.Assign (currentContainer.Get (0)));
    newInterfaces.Add (ip.Assign (currentContainer.Get (1)));

    auto interfaceAddress1 = newInterfaces.GetAddress (0);
    auto interfaceAddress2 = newInterfaces.GetAddress (1);
    uint32_t node1 = (currentContainer.Get (0))->GetNode()->GetId();
    uint32_t node2 = (currentContainer.Get (1))->GetNode()->GetId();

/*     if (m_systemId == 0)
      std::cout << i << "/" << m_devices.size () << "\n"; */
/* 	if (m_systemId == 0)
	  std::cout << "Node " << node1 << "(" << interfaceAddress1 << ") is connected with node  "
                << node2 << "(" << interfaceAddress2 << ")\n"; */

	m_nodesConnectionsIps[node1].push_back(interfaceAddress2);
	m_nodesConnectionsIps[node2].push_back(interfaceAddress1);

    ip.NewNetwork ();

    m_interfaces.push_back (newInterfaces);

	m_peersDownloadSpeeds[node1][interfaceAddress2] = m_nodesInternetSpeeds[node2].downloadSpeed;
	m_peersDownloadSpeeds[node2][interfaceAddress1] = m_nodesInternetSpeeds[node1].downloadSpeed;
	m_peersUploadSpeeds[node1][interfaceAddress2] = m_nodesInternetSpeeds[node2].uploadSpeed;
	m_peersUploadSpeeds[node2][interfaceAddress1] = m_nodesInternetSpeeds[node1].uploadSpeed;
  }


/*   //Print the nodes' connections
  if (m_systemId == 0)
  {
    std::cout << "The nodes connections are:" << std::endl;
    for(auto &node : m_nodesConnectionsIps)
    {
  	  std::cout << "\nNode " << node.first << ":    " ;
	  for(std::vector<Ipv4Address>::const_iterator it = node.second.begin(); it != node.second.end(); it++)
	  {
        std::cout  << "\t" << *it ;
	  }
    }
    std::cout << "\n" << std::endl;
  } */

  tFinish = GetWallTime();
  if (m_systemId == 0)
    std::cout << "The Ip addresses have been assigned in " << tFinish - tStart << "s.\n";
}


Ptr<Node>
BlockchainTopologyHelper::GetNode (uint32_t id)
{
  if (id > m_nodes.size () - 1 )
    {
      NS_FATAL_ERROR ("Index out of bounds in BlockchainTopologyHelper::GetNode.");
    }

  return (m_nodes.at (id)).Get (0);
}



Ipv4InterfaceContainer
BlockchainTopologyHelper::GetIpv4InterfaceContainer (void) const
{
  Ipv4InterfaceContainer ipv4InterfaceContainer;

  for (auto container = m_interfaces.begin(); container != m_interfaces.end(); container++)
    ipv4InterfaceContainer.Add(*container);

  return ipv4InterfaceContainer;
}


std::map<uint32_t, std::vector<Ipv4Address>>
BlockchainTopologyHelper::GetNodesConnectionsIps (void) const
{
  return m_nodesConnectionsIps;
}


std::vector<uint32_t>
BlockchainTopologyHelper::GetMiners (void) const
{
  return m_miners;
}

void
BlockchainTopologyHelper::AssignRegion (uint32_t id)
{
  auto index = std::find(m_miners.begin(), m_miners.end(), id);
  if ( index != m_miners.end() )
  {
    m_blockchainNodesRegion[id] = m_minersRegions[index - m_miners.begin()];
  }
  else{
    int number = m_nodesDistribution(m_generator);
    m_blockchainNodesRegion[id] = number;
  }

/*   if (m_systemId == 0)
    std::cout << "SystemId = " << m_systemId << " assigned node " << id << " in " << getBlockchainRegion(getBlockchainEnum(m_blockchainNodesRegion[id])) << "\n"; */
}


void
BlockchainTopologyHelper::AssignInternetSpeeds(uint32_t id)
{
  auto index = std::find(m_miners.begin(), m_miners.end(), id);
  if ( index != m_miners.end() )
  {
    m_nodesInternetSpeeds[id].downloadSpeed = m_minerDownloadSpeed;
    m_nodesInternetSpeeds[id].uploadSpeed = m_minerUploadSpeed;
  }
  else{
    m_nodesInternetSpeeds[id].downloadSpeed = m_minerDownloadSpeed;
    m_nodesInternetSpeeds[id].uploadSpeed = m_minerUploadSpeed;
    /*switch(m_blockchainNodesRegion[id])
    {
      case ASIA_PACIFIC:
      {
        m_nodesInternetSpeeds[id].downloadSpeed = m_asiaPacificDownloadBandwidthDistribution(m_generator);
        m_nodesInternetSpeeds[id].uploadSpeed = m_asiaPacificUploadBandwidthDistribution(m_generator);
        break;
      }
      case AUSTRALIA:
      {
        m_nodesInternetSpeeds[id].downloadSpeed = m_australiaDownloadBandwidthDistribution(m_generator);
        m_nodesInternetSpeeds[id].uploadSpeed = m_australiaUploadBandwidthDistribution(m_generator);
        break;
      }
      case EUROPE:
      {
        m_nodesInternetSpeeds[id].downloadSpeed = m_europeDownloadBandwidthDistribution(m_generator);
        m_nodesInternetSpeeds[id].uploadSpeed = m_europeUploadBandwidthDistribution(m_generator);
        break;
      }
      case AFRICA:
      {
        m_nodesInternetSpeeds[id].downloadSpeed = m_africaDownloadBandwidthDistribution(m_generator);
        m_nodesInternetSpeeds[id].uploadSpeed = m_africaUploadBandwidthDistribution(m_generator);
        break;
      }
      case NORTH_AMERICA:
      {
        m_nodesInternetSpeeds[id].downloadSpeed = m_northAmericaDownloadBandwidthDistribution(m_generator);
        m_nodesInternetSpeeds[id].uploadSpeed = m_northAmericaUploadBandwidthDistribution(m_generator);
        break;
      }
      case SOUTH_AMERICA:
      {
        m_nodesInternetSpeeds[id].downloadSpeed = m_southAmericaDownloadBandwidthDistribution(m_generator);
        m_nodesInternetSpeeds[id].uploadSpeed = m_southAmericaUploadBandwidthDistribution(m_generator);
        break;
      }
    }*/
  }

/*  if (m_systemId == 0)
    std::cout << "SystemId = " << m_systemId << " assigned node " << id << " in " << getBlockchainRegion(getBlockchainEnum(m_blockchainNodesRegion[id]))
              << " with download speed = " << m_nodesInternetSpeeds[id].downloadSpeed << " Mbps and upload speed " << m_nodesInternetSpeeds[id].uploadSpeed << " Mbps\n"; */
}


uint32_t*
BlockchainTopologyHelper::GetBlockchainNodesRegions (void)
{
  return m_blockchainNodesRegion;
}


std::map<uint32_t, std::map<Ipv4Address, double>>
BlockchainTopologyHelper::GetPeersDownloadSpeeds (void) const
{
  return m_peersDownloadSpeeds;
}


std::map<uint32_t, std::map<Ipv4Address, double>>
BlockchainTopologyHelper::GetPeersUploadSpeeds (void) const
{
  return m_peersUploadSpeeds;
}


std::map<uint32_t, nodeInternetSpeeds>
BlockchainTopologyHelper::GetNodesInternetSpeeds (void) const
{
  return m_nodesInternetSpeeds;
}

} // namespace ns3

static double GetWallTime()
{
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
