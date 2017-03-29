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
#include "ns3/constant-position-mobility-model.h"
#include "ns3/string.h"
#include "ns3/vector.h"
#include "ns3/log.h"
#include "ns3/ipv6-address-generator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"
#include "ns3/blockchain-constants.h"
#include <algorithm>
#include <fstream>
#include <time.h>
#include <sys/time.h>

static double GetWallTime();
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BlockchainTopologyHelper");

BlockchainTopologyHelper::BlockchainTopologyHelper (uint32_t noCpus, uint32_t totalNoNodes, uint32_t noMiners, enum BlockchainRegion *minersRegions,
                                               int minConnectionsPerNode, int maxConnectionsPerNode,
						                      double latencyParetoShapeDivider, uint32_t systemId, std::vector<uint32_t> miners, std::map<uint32_t>)
  : m_noCpus(noCpus), m_totalNoNodes (totalNoNodes), m_noMiners (noMiners),
    m_minConnectionsPerNode (minConnectionsPerNode), m_maxConnectionsPerNode (maxConnectionsPerNode),
	m_totalNoLinks (0), m_latencyParetoShapeDivider (latencyParetoShapeDivider),
	m_systemId (systemId), m_minConnectionsPerMiner (100), m_maxConnectionsPerMiner (800),
	m_minerDownloadSpeed (100), m_minerUploadSpeed (100)
{

  std::vector<uint32_t>     nodes;    //nodes contain the ids of the nodes
  double                    tStart = GetWallTime();
  double                    tFinish;


  for (int k = 0; k < 6; k++)
    for (int j = 0; j < 6; j++)
	  m_regionLatencies[k][j] = regionLatencies[k][j];

  m_regionDownloadSpeeds[NORTH_AMERICA] = 41.68;
  m_regionDownloadSpeeds[EUROPE] = 21.29;
  m_regionDownloadSpeeds[SOUTH_AMERICA] = 9.89;
  m_regionDownloadSpeeds[ASIA_PACIFIC] = 14.56;
  m_regionDownloadSpeeds[AFRICA] = 6.9;
  m_regionDownloadSpeeds[AUSTRALIA] = 16;

  m_regionUploadSpeeds[NORTH_AMERICA] = 6.74;
  m_regionUploadSpeeds[EUROPE] = 6.72;
  m_regionUploadSpeeds[SOUTH_AMERICA] = 2.2;
  m_regionUploadSpeeds[ASIA_PACIFIC] = 6.53;
  m_regionUploadSpeeds[AFRICA] = 1.7;
  m_regionUploadSpeeds[AUSTRALIA] = 6.1;
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


  std::array<double,7> nodesDistributionIntervals {NORTH_AMERICA, EUROPE, SOUTH_AMERICA, ASIA_PACIFIC, AFRICA, AUSTRALIA, OTHER};

      std::array<double,6> nodesDistributionWeights {39.24, 48.79, 2.12, 6.97, 1.06, 1.82};
      m_nodesDistribution = std::piecewise_constant_distribution<double> (nodesDistributionIntervals.begin(), nodesDistributionIntervals.end(), nodesDistributionWeights.begin());

  std::array<double,7> connectionsDistributionIntervals {1, 5, 10, 15, 20, 30, 125};
  for (int i = 0; i < 7; i++)
	connectionsDistributionIntervals[i] -= i;

  std::array<double,6> connectionsDistributionWeights {10, 40, 30, 13, 6, 1};

  m_connectionsDistribution = std::piecewise_constant_distribution<double> (connectionsDistributionIntervals.begin(), connectionsDistributionIntervals.end(), connectionsDistributionWeights.begin());

  m_europeDownloadBandwidthDistribution = std::piecewise_constant_distribution<double> (downloadBandwitdhIntervals.begin(), downloadBandwitdhIntervals.end(), EuropeDownloadWeights.begin());
  m_europeUploadBandwidthDistribution = std::piecewise_constant_distribution<double> (uploadBandwitdhIntervals.begin(), uploadBandwitdhIntervals.end(), EuropeUploadWeights.begin());
  m_northAmericaDownloadBandwidthDistribution = std::piecewise_constant_distribution<double> (downloadBandwitdhIntervals.begin(), downloadBandwitdhIntervals.end(), NorthAmericaDownloadWeights.begin());
  m_northAmericaUploadBandwidthDistribution = std::piecewise_constant_distribution<double> (uploadBandwitdhIntervals.begin(), uploadBandwitdhIntervals.end(), NorthAmericaUploadWeights.begin());
  m_asiaPacificDownloadBandwidthDistribution = std::piecewise_constant_distribution<double> (downloadBandwitdhIntervals.begin(), downloadBandwitdhIntervals.end(), AsiaPacificDownloadWeights.begin());
  m_asiaPacificUploadBandwidthDistribution = std::piecewise_constant_distribution<double> (uploadBandwitdhIntervals.begin(), uploadBandwitdhIntervals.end(), AsiaPacificUploadWeights.begin());
  m_africaDownloadBandwidthDistribution = std::piecewise_constant_distribution<double> (downloadBandwitdhIntervals.begin(), downloadBandwitdhIntervals.end(), AfricaDownloadWeights.begin());
  m_africaUploadBandwidthDistribution = std::piecewise_constant_distribution<double> (uploadBandwitdhIntervals.begin(), uploadBandwitdhIntervals.end(), AfricaUploadWeights.begin());
  m_southAmericaDownloadBandwidthDistribution = std::piecewise_constant_distribution<double> (downloadBandwitdhIntervals.begin(), downloadBandwitdhIntervals.end(), SouthAmericaDownloadWeights.begin());
  m_southAmericaUploadBandwidthDistribution = std::piecewise_constant_distribution<double> (uploadBandwitdhIntervals.begin(), uploadBandwitdhIntervals.end(), SouthAmericaUploadWeights.begin());
  m_australiaDownloadBandwidthDistribution = std::piecewise_constant_distribution<double> (downloadBandwitdhIntervals.begin(), downloadBandwitdhIntervals.end(), AustraliaDownloadWeights.begin());
  m_australiaUploadBandwidthDistribution = std::piecewise_constant_distribution<double> (uploadBandwitdhIntervals.begin(), uploadBandwitdhIntervals.end(), AustraliaUploadWeights.begin());

  m_minersRegions = new enum BlockchainRegion[m_noMiners];
  for (int i = 0; i < m_noMiners; i++)
  {
    m_minersRegions[i] = minersRegions[i];
  }

  /**
   * Create a vector containing all the nodes ids
   */
  for (int i = 0; i < m_totalNoNodes; i++)
  {
    nodes.push_back(i);
  }

/*   //Print the initialized nodes
  if (m_systemId == 0)
  {
    for (std::vector<uint32_t>::iterator j = nodes.begin(); j != nodes.end(); j++)
    {
	  std::cout << *j << " " ;
    }
  } */

  //Choose the miners randomly. They should be unique (no miner should be chosen twice).
  //So, remove each chose miner from nodes vector
  for (int i = 0; i < noMiners; i++)
  {
    uint32_t index = rand() % nodes.size();
    m_miners.push_back(nodes[index]);

/*     if (m_systemId == 0)
      std::cout << "\n" << "Chose " << nodes[index] << "     "; */

    nodes.erase(nodes.begin() + index);

/* 	if (m_systemId == 0)
	{
      for (std::vector<uint32_t>::iterator it = nodes.begin(); it != nodes.end(); it++)
      {
	    std::cout << *it << " " ;
      }
	} */
  }

  sort(m_miners.begin(), m_miners.end());

/*   //Print the miners
  if (m_systemId == 0)
  {
    std::cout << "\n\nThe miners are:\n";
    for (std::vector<uint32_t>::iterator j = m_miners.begin(); j != m_miners.end(); j++)
    {
	  std::cout << *j << " " ;
    }
    std::cout << "\n\n";
  } */

  //Interconnect the miners
  for(auto &miner : m_miners)
  {
    for(auto &peer : m_miners)
    {
      if (miner != peer)
        m_nodesConnections[miner].push_back(peer);
	}
  }


/*   //Print the miners' connections
  if (m_systemId == 0)
  {
    std::cout << "The miners are interconnected:";
    for(auto &miner : m_nodesConnections)
    {
	  std::cout << "\nMiner " << miner.first << ":\t" ;
	  for(std::vector<uint32_t>::const_iterator it = miner.second.begin(); it != miner.second.end(); it++)
	  {
        std::cout << *it << "\t" ;
	  }
    }
    std::cout << "\n" << std::endl;
  } */

  //Interconnect the nodes

  //nodes contain the ids of the nodes
  nodes.clear();

  for (int i = 0; i < m_totalNoNodes; i++)
  {
    nodes.push_back(i);
  }


  for(int i = 0; i < m_totalNoNodes; i++)
  {
	int count = 0;
	int minConnections;
	int maxConnections;

	if ( std::find(m_miners.begin(), m_miners.end(), i) != m_miners.end() )
    {
      m_minConnections[i] = m_minConnectionsPerMiner;
      m_maxConnections[i] = m_maxConnectionsPerMiner;
    }
	else
	{
      if (m_minConnectionsPerNode > 0 && m_maxConnectionsPerNode > 0)
      {
	    minConnections = m_minConnectionsPerNode;
	    maxConnections = m_maxConnectionsPerNode;
      }
      else
	  {
	    minConnections = static_cast<int>(m_connectionsDistribution(m_generator));
	    if (minConnections < 1)
	      minConnections = 1;

	    int index = 0;
        for (int k = 1; k < connectionsDistributionIntervals.size(); k++)
        {
          if (minConnections < connectionsDistributionIntervals[k])
          {
            index = k;
            break;
          }
		}
        maxConnections = minConnections + index;
	  }
	  m_minConnections[i] = minConnections;
	  m_maxConnections[i] = maxConnections;
	}
  }

  //First the miners
  for(auto &i : m_miners)
  {
	int count = 0;

    while (m_nodesConnections[i].size() < m_minConnections[i] && count < 10*m_minConnections[i])
    {
      uint32_t index = rand() % nodes.size();
	  uint32_t candidatePeer = nodes[index];

      if (candidatePeer == i)
      {
/* 		if (m_systemId == 0)
          std::cout << "Node " << i << " does not need a connection with itself" << "\n"; */
      }
      else if (std::find(m_nodesConnections[i].begin(), m_nodesConnections[i].end(), candidatePeer) != m_nodesConnections[i].end())
      {
/* 		if (m_systemId == 0)
          std::cout << "Node " << i << " has already a connection to Node " << nodes[index] << "\n"; */
      }
      else if (m_nodesConnections[candidatePeer].size() >= m_maxConnections[candidatePeer])
      {
/* 		if (m_systemId == 0)
          std::cout << "Node " << nodes[index] << " has already " << m_maxConnections[candidatePeer] << " connections" << "\n"; */
      }
      else
      {
        m_nodesConnections[i].push_back(candidatePeer);
        m_nodesConnections[candidatePeer].push_back(i);

        if (m_nodesConnections[candidatePeer].size() == m_maxConnections[candidatePeer])
        {
/* 		  if (m_systemId == 0)
            std::cout << "Node " << nodes[index] << " is removed from index\n"; */
          nodes.erase(nodes.begin() + index);
        }
      }
      count++;
	}
  }

  //Then the rest of nodes
  for(int i = 0; i < m_totalNoNodes; i++)
  {
	int count = 0;

    while (m_nodesConnections[i].size() < m_minConnections[i] && count < 10*m_minConnections[i])
    {
      uint32_t index = rand() % nodes.size();
	  uint32_t candidatePeer = nodes[index];

      if (candidatePeer == i)
      {
/* 		if (m_systemId == 0)
          std::cout << "Node " << i << " does not need a connection with itself" << "\n"; */
      }
      else if (std::find(m_nodesConnections[i].begin(), m_nodesConnections[i].end(), candidatePeer) != m_nodesConnections[i].end())
      {
/* 		if (m_systemId == 0)
          std::cout << "Node " << i << " has already a connection to Node " << nodes[index] << "\n"; */
      }
      else if (m_nodesConnections[candidatePeer].size() >= m_maxConnections[candidatePeer])
      {
/* 		if (m_systemId == 0)
          std::cout << "Node " << nodes[index] << " has already " << m_maxConnections[candidatePeer] << " connections" << "\n"; */
      }
      else
      {
        m_nodesConnections[i].push_back(candidatePeer);
        m_nodesConnections[candidatePeer].push_back(i);

        if (m_nodesConnections[candidatePeer].size() == m_maxConnections[candidatePeer])
        {
/* 		  if (m_systemId == 0)
            std::cout << "Node " << nodes[index] << " is removed from index\n"; */
          nodes.erase(nodes.begin() + index);
        }
      }
      count++;
	}
  }

  //Print the nodes with fewer than required connections
  if (m_systemId == 0)
  {
    for(int i = 0; i < m_totalNoNodes; i++)
    {
	  if (m_nodesConnections[i].size() < m_minConnections[i])
	    std::cout << "Node " << i << " should have at least " << m_minConnections[i] << " connections but it has only " << m_nodesConnections[i].size() << " connections\n";
    }
  }

/*   //Print the nodes' connections
  if (m_systemId == 0)
  {
    std::cout << "The nodes connections are:" << std::endl;
    for(auto &node : m_nodesConnections)
    {
  	  std::cout << "\nNode " << node.first << ":    " ;
	  for(std::vector<uint32_t>::const_iterator it = node.second.begin(); it != node.second.end(); it++)
	  {
        std::cout  << "\t" << *it;
	  }
    }
    std::cout << "\n" << std::endl;
  } */

  //Print the nodes' connections distribution
  if (m_systemId == 0)
  {
    int *intervals =  new int[connectionsDistributionIntervals.size() + 1];
	int *stats = new int[connectionsDistributionIntervals.size()];
	double averageNoConnectionsPerNode = 0;
	double averageNoConnectionsPerMiner = 0;

	for(int i = 0; i < connectionsDistributionIntervals.size(); i++)
      intervals[i] = connectionsDistributionIntervals[i] + i;
    intervals[connectionsDistributionIntervals.size()] = m_maxConnectionsPerMiner;

	for(int i = 0; i < connectionsDistributionIntervals.size(); i++)
      stats[i] = 0;

    std::cout << "\nThe nodes connections stats are:\n";
    for(auto &node : m_nodesConnections)
    {
  	  //std::cout << "\nNode " << node.first << ": " << m_minConnections[node.first] << ", " << m_maxConnections[node.first] << ", " << node.second.size();
      bool placed = false;

      if ( std::find(m_miners.begin(), m_miners.end(), node.first) == m_miners.end() )
        averageNoConnectionsPerNode += node.second.size();
      else
        averageNoConnectionsPerMiner += node.second.size();

	  for (int i = 1; i < connectionsDistributionIntervals.size(); i++)
      {
        if (node.second.size() <= intervals[i])
        {
          stats[i-1]++;
          placed = true;
          break;
		}
      }
	  if (!placed)
      {
        //std::cout << "Node " << node.first << " has " << node.second.size() << " connections\n";
        stats[connectionsDistributionIntervals.size() - 1]++;
      }
    }

    std::cout << "Average Number of Connections Per Node = " << averageNoConnectionsPerNode / (m_totalNoNodes - m_noMiners)
	          << "\nAverage Number of Connections Per Miner = " << averageNoConnectionsPerMiner / (m_noMiners) << "\nConnections distribution: \n";

    for (uint32_t i = 0; i < connectionsDistributionIntervals.size(); i++)
    {
      std::cout << intervals[i] << "-" << intervals[i+1] << ": " << stats[i] << "(" << stats[i] * 100.0 / m_totalNoNodes << "%)\n";
    }

    delete[] intervals;
	delete[] stats;
  }

  tFinish = GetWallTime();
  if (m_systemId == 0)
  {
    std::cout << "The nodes connections were created in " << tFinish - tStart << "s.\n";
    std::cout << "The minimum number of connections for each node is " << m_minConnectionsPerNode
              << " and whereas the maximum is " << m_maxConnectionsPerNode << ".\n";
  }


  InternetStackHelper stack;

  std::ostringstream latencyStringStream;
  std::ostringstream bandwidthStream;

  PointToPointHelper pointToPoint;

  tStart = GetWallTime();
  //Create the blockchain nodes
  for (uint32_t i = 0; i < m_totalNoNodes; i++)
  {
    NodeContainer currentNode;
    currentNode.Create (1, i % m_noCpus);
/* 	if (m_systemId == 0)
      std::cout << "Creating a node with Id = " << i << " and systemId = " << i % m_noCpus << "\n"; */
    m_nodes.push_back (currentNode);
	AssignRegion(i);
    AssignInternetSpeeds(i);
  }


  //Print region bandwidths averages
  if (m_systemId == 0)
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
  }

  tFinish = GetWallTime();
  if (m_systemId == 0)
    std::cout << "The nodes were created in " << tFinish - tStart << "s.\n";

  tStart = GetWallTime();

  //Create first the links between miners
  for(auto miner = m_miners.begin(); miner != m_miners.end(); miner++)
  {

    for(std::vector<uint32_t>::const_iterator it = m_nodesConnections[*miner].begin(); it != m_nodesConnections[*miner].begin() + m_miners.size() - 1; it++)
    {
      if ( *it > *miner)	//Do not recreate links
      {
        NetDeviceContainer newDevices;

        m_totalNoLinks++;

		double bandwidth = std::min(std::min(m_nodesInternetSpeeds[m_nodes.at (*miner).Get (0)->GetId()].uploadSpeed,
                                    m_nodesInternetSpeeds[m_nodes.at (*miner).Get (0)->GetId()].downloadSpeed),
                                    std::min(m_nodesInternetSpeeds[m_nodes.at (*it).Get (0)->GetId()].uploadSpeed,
                                    m_nodesInternetSpeeds[m_nodes.at (*it).Get (0)->GetId()].downloadSpeed));
		bandwidthStream.str("");
        bandwidthStream.clear();
		bandwidthStream << bandwidth << "Mbps";

        latencyStringStream.str("");
        latencyStringStream.clear();

		if (m_latencyParetoShapeDivider > 0)
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
        }


		pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidthStream.str()));
		pointToPoint.SetChannelAttribute ("Delay", StringValue (latencyStringStream.str()));

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

  for(auto &node : m_nodesConnections)
  {

    for(std::vector<uint32_t>::const_iterator it = node.second.begin(); it != node.second.end(); it++)
    {

      if ( *it > node.first && (std::find(m_miners.begin(), m_miners.end(), *it) == m_miners.end() ||
	       std::find(m_miners.begin(), m_miners.end(), node.first) == m_miners.end()))	//Do not recreate links
      {
        NetDeviceContainer newDevices;

        m_totalNoLinks++;

		double bandwidth = std::min(std::min(m_nodesInternetSpeeds[m_nodes.at (node.first).Get (0)->GetId()].uploadSpeed,
                                    m_nodesInternetSpeeds[m_nodes.at (node.first).Get (0)->GetId()].downloadSpeed),
                                    std::min(m_nodesInternetSpeeds[m_nodes.at (*it).Get (0)->GetId()].uploadSpeed,
                                    m_nodesInternetSpeeds[m_nodes.at (*it).Get (0)->GetId()].downloadSpeed));
		bandwidthStream.str("");
        bandwidthStream.clear();
		bandwidthStream << bandwidth << "Mbps";

        latencyStringStream.str("");
        latencyStringStream.clear();

		if (m_latencyParetoShapeDivider > 0)
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
        }

		pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidthStream.str()));
		pointToPoint.SetChannelAttribute ("Delay", StringValue (latencyStringStream.str()));

        newDevices.Add (pointToPoint.Install (m_nodes.at (node.first).Get (0), m_nodes.at (*it).Get (0)));
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
    switch(m_blockchainNodesRegion[id])
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
    }
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
