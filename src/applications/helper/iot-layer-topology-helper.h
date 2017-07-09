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
 * Author: Saptarshi Gan <sapedu11@gmail.com>
 */

#ifndef IOT_LAYER_TOPOLOGY_HELPER_H
#define IOT_LAYER_TOPOLOGY_HELPER_H

#include <vector>

#include "internet-stack-helper.h"
#include "point-to-point-helper.h"
#include "ipv4-address-helper.h"
#include "ipv6-address-helper.h"
#include "ipv4-interface-container.h"
#include "ipv6-interface-container.h"
#include "net-device-container.h"
#include "ipv6-address-helper-custom.h"
#include "ns3/blockchain.h"
#include <random>

namespace ns3 {

/**
 * \ingroup point-to-point-layout
 *
 * \brief A helper to make it easier to create the discussed layer topology
 * with p2p links
 */
class IoTLayerTopologyHelper
{
public:
  /**
   * Create a IoTLayerTopologyHelper in order to easily create
   * discussed layer topology with p2p links
   *
   */
  IoTLayerTopologyHelper (uint32_t noCpus, uint32_t totalNoNodes, enum ManufacturerID *manufacturers,
                         int minConnectionsPerNode, int maxConnectionsPerNode,
                         double latencyParetoShapeDivider, uint32_t systemId, std::vector<uint32_t> validators, std::map<uint32_t,std::vector<uint32_t> > gatewayChildMap, std::map<uint32_t, uint32_t> gatewayValidatorMap, std::map<uint32_t, std::vector<uint32_t> > validatorLinkMap);

  ~IoTLayerTopologyHelper ();

  Ptr<Node> GetNode (uint32_t id);

  /**
   * This returns an Ipv6 address at the node specified by
   * the (row, col) address.  Technically, a node will have
   * multiple interfaces in the grid; therefore, it also has
   * multiple Ipv6 addresses.  This method only returns one of
   * the addresses. If you picture the grid, the address returned
   * is the left row device of all the nodes, except the left-most
   * grid nodes, which returns the right row device.
   *
   * \param row the row address of the node desired
   *
   * \param col the column address of the node desired
   *
   * \returns Ipv6Address of one of the interfaces of the node
   *          specified by the (row, col) address
   */
  Ipv6Address GetIpv6Address (uint32_t row, uint32_t col);


  /**
   * \param stack an InternetStackHelper which is used to install
   *              on every node in the grid
   */
  void InstallStack (InternetStackHelper stack);

  /**
   * Assigns Ipv6 addresses to all the row and column interfaces
   *
   * \param ip the Ipv6AddressHelper used to assign Ipv6 addresses
   *              to all of the row interfaces in the grid
   *
   * \param ip the Ipv6AddressHelper used to assign Ipv6 addresses
   *              to all of the row interfaces in the grid
   */
  void AssignIpv6Addresses (Ipv6AddressHelperCustom ip);


  /**
   * Sets up the node canvas locations for every node in the grid.
   * This is needed for use with the animation interface
   *
   * \param ulx upper left x value
   * \param uly upper left y value
   * \param lrx lower right x value
   * \param lry lower right y value
   */
  void BoundingBox (double ulx, double uly, double lrx, double lry);

  /**
   * Get the interface container
   */
   Ipv6InterfaceContainer GetIpv6InterfaceContainer (void) const;

   std::map<uint32_t, std::vector<Ipv6Address>> GetNodesConnectionsIps (void) const;

   std::vector<uint32_t> GetValidators (void) const;

   uint32_t* GetBlockchainNodesRegions (void);

   std::map<uint32_t, std::map<Ipv6Address, double>> GetPeersDownloadSpeeds(void) const;
   std::map<uint32_t, std::map<Ipv6Address, double>> GetPeersUploadSpeeds(void) const;

   std::map<uint32_t, nodeInternetSpeeds> GetNodesInternetSpeeds (void) const;

private:

  void AssignRegion (uint32_t id);
  void AssignInternetSpeeds(uint32_t id);

  uint32_t     m_totalNoNodes;                  //!< The total number of nodes
  uint32_t     m_noValidators;                      //!< The total number of validators
  uint32_t     m_noCpus;                        //!< The number of the available cpus in the simulation
  double       m_latencyParetoShapeDivider;     //!<  The pareto shape for the latency of the point-to-point links
  int          m_minConnectionsPerNode;         //!<  The minimum connections per node
  int          m_maxConnectionsPerNode;         //!<  The maximum connections per node
  int          m_minConnectionsPerValidator;        //!<  The minimum connections per node
  int          m_maxConnectionsPerValidator;        //!<  The maximum connections per node
  double       m_validatorDownloadSpeed;            //!<  The download speed of validators
  double       m_validatorUploadSpeed;              //!<  The upload speed of validators
  uint32_t     m_totalNoLinks;                  //!<  Total number of links
  uint32_t     m_systemId;

  enum ManufacturerID                             *m_manufacturers;
  std::vector<uint32_t>                           m_validators;                  //!< The ids of the validators
  std::map<uint32_t, std::vector<uint32_t>>       m_nodesConnections;        //!< key = nodeId
  std::map<uint32_t, std::vector<Ipv6Address>>    m_nodesConnectionsIps;     //!< key = nodeId
  std::map<uint32_t, std::vector<uint32_t>>    m_nodeGatewayMap;            //!< key = nodeId
  std::map<uint32_t, uint32_t>                 m_gatewayValidatorMap;           //!< key=nodeId
  std::map<uint32_t, std::vector<uint32_t>>    m_validatorLinkMap;            //!< key = nodeId
  std::vector<NodeContainer>                      m_nodes;                   //!< all the nodes in the network
  std::vector<NetDeviceContainer>                 m_devices;                 //!< NetDevices in the network
  std::vector<Ipv6InterfaceContainer>             m_interfaces;              //!< Ipv6 interfaces in the network
  uint32_t                                       *m_blockchainNodesRegion;      //!< The region in which the blockchain nodes are located
  double                                          m_regionLatencies[6][6];   //!< The inter- and intra-region latencies
  double                                          m_regionDownloadSpeeds[6];
  double                                          m_regionUploadSpeeds[6];


  std::map<uint32_t, std::map<Ipv6Address, double>>    m_peersDownloadSpeeds;     //!< key1 = nodeId, key2 = Ipv6Address of peer
  std::map<uint32_t, std::map<Ipv6Address, double>>    m_peersUploadSpeeds;       //!< key1 = nodeId, key2 = Ipv6Address of peer
  std::map<uint32_t, nodeInternetSpeeds>               m_nodesInternetSpeeds;     //!< key = nodeId
  std::map<uint32_t, int>                              m_minConnections;          //!< key = nodeId
  std::map<uint32_t, int>                              m_maxConnections;          //!< key = nodeId

  std::default_random_engine                     m_generator;
  std::piecewise_constant_distribution<double>   m_nodesDistribution;
  std::piecewise_constant_distribution<double>   m_connectionsDistribution;
  std::piecewise_constant_distribution<double>   m_europeDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_europeUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_northAmericaDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_northAmericaUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_asiaPacificDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_asiaPacificUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_africaDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_africaUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_southAmericaDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_southAmericaUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_australiaDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_australiaUploadBandwidthDistribution;
};



} // namespace ns3

#endif /* BLOCKCHAIN_TOPOLOGY_HELPER_H */
