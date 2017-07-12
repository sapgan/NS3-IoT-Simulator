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
 * Author: Saptarshi Gan<sapedu11@gmail.com>
 */

#include "ns3/iot-flat-topology-helper.h"
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

NS_LOG_COMPONENT_DEFINE ("IoTFlatTopologyHelper");

IoTFlatTopologyHelper::IoTFlatTopologyHelper (uint32_t noCpus, uint32_t totalNoNodes, enum ManufacturerID *manufacturers,
                                               int minConnectionsPerNode, int maxConnectionsPerNode,
						                      double latencyParetoShapeDivider, uint32_t systemId, std::vector<uint32_t> validators, std::map<uint32_t, uint32_t> iotValidatorMap, std::map<uint32_t, std::vector<uint32_t> > validatorLinkMap)
  : m_noCpus(noCpus), m_totalNoNodes(totalNoNodes), m_minConnectionsPerNode (minConnectionsPerNode), m_maxConnectionsPerNode (maxConnectionsPerNode),
	m_totalNoLinks (0), m_latencyParetoShapeDivider (latencyParetoShapeDivider),
	m_systemId (systemId), m_minConnectionsPerValidator (10), m_maxConnectionsPerValidator (80),
	m_validatorDownloadSpeed (100), m_validatorUploadSpeed (100), m_validators (validators) , m_iotValidatorMap (iotValidatorMap), m_validatorLinkMap (validatorLinkMap)
{
  std::vector<uint32_t>     nodes;    //nodes contain the ids of all the nodes
  double                    tStart = GetWallTime();
  double                    tFinish;

  int m_noValidators = validators.size();
  m_minConnectionsPerValidator = m_noValidators*(m_noValidators/2);
  m_maxConnectionsPerValidator = m_minConnectionsPerValidator ;
  //std::map<uint32_t, std::vector<uint32_t>>  m_nodesConnections;

  srand (1000);

  // Bounds check
  if (m_noValidators > m_totalNoNodes)
  {
    NS_FATAL_ERROR ("The number of validators is larger than the total number of nodes\n");
  }

  if (m_noValidators < 1)
  {
    NS_FATAL_ERROR ("You need at least one validator\n");
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
  m_manufacturers = new enum ManufacturerID[m_noValidators];
  for (int i = 0; i < m_noValidators; i++)
  {
    m_manufacturers[m_validators[i]] = manufacturers[m_validators[i]];
  }

  std::cout << "Creating list of all nodes!" << std::endl;
  /**
   * Create a vector containing all the nodes ids
   */
  for (int i = 1; i <= m_totalNoNodes; i++)
  {
    nodes.push_back(i);
  }

  // std::cout << "Forming the CCA and validator links!" << std::endl;
  std::cout << "Forming the validators links!" << std::endl;
  sort(m_validators.begin(), m_validators.end());

  //Interconnect the validators
  // for(auto &validator : m_validators)
  // {
  //   for(auto &peer : m_validators)
  //   {
  //     if (validator != peer)
  //       m_nodesConnections[validator].push_back(peer);
	// }
  // }

  std::map<uint32_t, std::vector<uint32_t> >::iterator nodeLink_it;

  for(nodeLink_it=m_validatorLinkMap.begin();nodeLink_it!=m_validatorLinkMap.end();nodeLink_it++){
    uint32_t validatorId = nodeLink_it->first;
    std::vector<uint32_t> peers = nodeLink_it->second;
    for(auto &peer: peers){
      m_nodesConnections[validatorId].push_back(peer);
    }
  }

    std::cout << "Forming validator and device links!" << std::endl;
  std::map<uint32_t, uint32_t >::iterator nodeLinkMap_it;
  //
  for(nodeLinkMap_it=m_iotValidatorMap.begin();nodeLinkMap_it!=m_iotValidatorMap.end();nodeLinkMap_it++)
  {
    uint32_t iotId = nodeLinkMap_it->first;
    uint32_t validator = nodeLinkMap_it->second;
    m_nodesConnections[iotId].push_back(validator);
    m_nodesConnections[validator].push_back(iotId);
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
      if ( std::find(m_validators.begin(), m_validators.end(), i) == m_validators.end())
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

      std::cout << "The download speed for region " << getManufacturerID(getBlockchainEnum(region.first)) << " = " << average / region.second.size() << " Mbps\n";
    }

    for (auto region : uploadRegionBandwidths)
    {
       double average = 0;
       for (auto &speed : region.second)
       {
         average += speed;
	   }

      std::cout << "The upload speed for region " << getManufacturerID(getBlockchainEnum(region.first)) << " = " << average / region.second.size() << " Mbps\n";
    }
  }*/

  tFinish = GetWallTime();
  if (m_systemId == 0)
    std::cout << "The nodes were created in " << tFinish - tStart << "s.\n";

  tStart = GetWallTime();

  std::cout << "Creating the links between the validator nodes!" << std::endl;
  //Create first the links between validators
  for(auto validator = m_validators.begin(); validator != m_validators.end(); validator++)
  {

    for(std::vector<uint32_t>::const_iterator it = m_nodesConnections[*validator].begin(); it != m_nodesConnections[*validator].end(); it++)
    {
      if(*it > m_noValidators)
        break;
      if ( *it > *validator)	//Do not recreate links and don't create links between validator and IoT node
      {
        NetDeviceContainer newDevices;

        m_totalNoLinks++;

		/*double bandwidth = std::min(std::min(m_nodesInternetSpeeds[m_nodes.at (*validator).Get (0)->GetId()].uploadSpeed,
                                    m_nodesInternetSpeeds[m_nodes.at (*validator).Get (0)->GetId()].downloadSpeed),
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
          paretoDistribution->SetAttribute ("Mean", DoubleValue (m_regionLatencies[m_blockchainNodesRegion[(m_nodes.at (*validator).Get (0))->GetId()]]
                                                                                  [m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]]));
          paretoDistribution->SetAttribute ("Shape", DoubleValue (m_regionLatencies[m_blockchainNodesRegion[(m_nodes.at (*validator).Get (0))->GetId()]]
                                                                                   [m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]] / m_latencyParetoShapeDivider));
          latencyStringStream << paretoDistribution->GetValue() << "ms";
        }
        else
        {
          latencyStringStream << m_regionLatencies[m_blockchainNodesRegion[(m_nodes.at (*validator).Get (0))->GetId()]]
                                                  [m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]] << "ms";
        }*/


		pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidthStream.str()));
		//pointToPoint.SetChannelAttribute ("Delay", StringValue (latencyStringStream.str()));

    newDevices.Add (pointToPoint.Install (m_nodes.at (*validator).Get (0), m_nodes.at (*it).Get (0)));
		m_devices.push_back (newDevices);
/* 		if (m_systemId == 0)
          std::cout << "Creating link " << m_totalNoLinks << " between nodes "
                    << (m_nodes.at (*validator).Get (0))->GetId() << " ("
                    <<  getManufacturerID(getBlockchainEnum(m_blockchainNodesRegion[(m_nodes.at (*validator).Get (0))->GetId()]))
                    << ") and node " << (m_nodes.at (*it).Get (0))->GetId() << " ("
                    <<  getManufacturerID(getBlockchainEnum(m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]))
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

      if ( *it > node.first && (std::find(m_validators.begin(), m_validators.end(), *it) == m_validators.end() ||
	       std::find(m_validators.begin(), m_validators.end(), node.first) == m_validators.end()))	//Do not recreate links
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
                    <<  getManufacturerID(getBlockchainEnum(m_blockchainNodesRegion[(m_nodes.at (node.first).Get (0))->GetId()]))
                    << ") and node " << (m_nodes.at (*it).Get (0))->GetId() << " ("
                    <<  getManufacturerID(getBlockchainEnum(m_blockchainNodesRegion[(m_nodes.at (*it).Get (0))->GetId()]))
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

IoTFlatTopologyHelper::~IoTFlatTopologyHelper ()
{
  delete[] m_blockchainNodesRegion;
  delete[] m_manufacturers;
}

void
IoTFlatTopologyHelper::InstallStack (InternetStackHelper stack)
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
IoTFlatTopologyHelper::AssignIpv6Addresses (Ipv6AddressHelperCustom ip)
{
  double tStart = GetWallTime();
  double tFinish;

  // Assign addresses to all devices in the network.
  // These devices are stored in a vector.
  for (uint32_t i = 0; i < m_devices.size (); ++i)
  {
    Ipv6InterfaceContainer newInterfaces;
    NetDeviceContainer currentContainer = m_devices[i];

    newInterfaces.Add (ip.Assign (currentContainer.Get (0)));
    newInterfaces.Add (ip.Assign (currentContainer.Get (1)));

    auto interfaceAddress1 = newInterfaces.GetAddress (0, 0);
    auto interfaceAddress2 = newInterfaces.GetAddress (1, 1);
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
	  for(std::vector<Ipv6Address>::const_iterator it = node.second.begin(); it != node.second.end(); it++)
	  {
        std::cout  << "\t" << *it ;
	  }
    }
    std::cout << "\n" << std::endl;
  } */

  tFinish = GetWallTime();
  if (m_systemId == 0)
    std::cout << "The IP addresses have been assigned in " << tFinish - tStart << "s.\n";
}


Ptr<Node>
IoTFlatTopologyHelper::GetNode (uint32_t id)
{
  if (id > m_nodes.size () - 1 )
    {
      NS_FATAL_ERROR ("Index out of bounds in IoTFlatTopologyHelper::GetNode.");
    }

  return (m_nodes.at (id)).Get (0);
}



Ipv6InterfaceContainer
IoTFlatTopologyHelper::GetIpv6InterfaceContainer (void) const
{
  Ipv6InterfaceContainer ipv6InterfaceContainer;

  for (auto container = m_interfaces.begin(); container != m_interfaces.end(); container++)
    ipv6InterfaceContainer.Add(*container);

  return ipv6InterfaceContainer;
}


std::map<uint32_t, std::vector<Ipv6Address>>
IoTFlatTopologyHelper::GetNodesConnectionsIps (void) const
{
  return m_nodesConnectionsIps;
}


std::vector<uint32_t>
IoTFlatTopologyHelper::GetValidators (void) const
{
  return m_validators;
}

void
IoTFlatTopologyHelper::AssignRegion (uint32_t id)
{
  auto index = std::find(m_validators.begin(), m_validators.end(), id);
  if ( index != m_validators.end() )
  {
    m_blockchainNodesRegion[id] = m_manufacturers[index - m_validators.begin()];
  }
  else{
    int number = m_nodesDistribution(m_generator);
    m_blockchainNodesRegion[id] = number;
  }

/*   if (m_systemId == 0)
    std::cout << "SystemId = " << m_systemId << " assigned node " << id << " in " << getManufacturerID(getBlockchainEnum(m_blockchainNodesRegion[id])) << "\n"; */
}


void
IoTFlatTopologyHelper::AssignInternetSpeeds(uint32_t id)
{
  auto index = std::find(m_validators.begin(), m_validators.end(), id);
  if ( index != m_validators.end() )
  {
    m_nodesInternetSpeeds[id].downloadSpeed = m_validatorDownloadSpeed;
    m_nodesInternetSpeeds[id].uploadSpeed = m_validatorUploadSpeed;
  }
  else{
    m_nodesInternetSpeeds[id].downloadSpeed = m_validatorDownloadSpeed;
    m_nodesInternetSpeeds[id].uploadSpeed = m_validatorUploadSpeed;
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
    std::cout << "SystemId = " << m_systemId << " assigned node " << id << " in " << getManufacturerID(getBlockchainEnum(m_blockchainNodesRegion[id]))
              << " with download speed = " << m_nodesInternetSpeeds[id].downloadSpeed << " Mbps and upload speed " << m_nodesInternetSpeeds[id].uploadSpeed << " Mbps\n"; */
}


uint32_t*
IoTFlatTopologyHelper::GetBlockchainNodesRegions (void)
{
  return m_blockchainNodesRegion;
}


std::map<uint32_t, std::map<Ipv6Address, double>>
IoTFlatTopologyHelper::GetPeersDownloadSpeeds (void) const
{
  return m_peersDownloadSpeeds;
}


std::map<uint32_t, std::map<Ipv6Address, double>>
IoTFlatTopologyHelper::GetPeersUploadSpeeds (void) const
{
  return m_peersUploadSpeeds;
}


std::map<uint32_t, nodeInternetSpeeds>
IoTFlatTopologyHelper::GetNodesInternetSpeeds (void) const
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
