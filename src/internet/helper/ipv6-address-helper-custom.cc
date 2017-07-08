/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 University of Washington
 *
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
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/loopback-net-device.h"
#include "ns3/mac16-address.h"
#include "ns3/mac48-address.h"
#include "ns3/mac64-address.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-address-generator.h"
#include "ns3/simulator.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/traffic-control-layer.h"
#include "ipv6-address-helper-custom.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv6AddressHelperCustom");

Ipv6AddressHelperCustom::Ipv6AddressHelperCustom (bool checkAddressDuplication) : m_checkAddressDuplication (checkAddressDuplication)
{
  NS_LOG_FUNCTION (this);
  Ipv6AddressGenerator::Init (Ipv6Address ("2001:db8::"), Ipv6Prefix (64));
}

Ipv6AddressHelperCustom::Ipv6AddressHelperCustom (
  const Ipv6Address network,
  const Ipv6Prefix prefix,
  bool  checkAddressDuplication,
  const Ipv6Address base
) : m_checkAddressDuplication (checkAddressDuplication)
{
  NS_LOG_FUNCTION (this << network << prefix << base);
  Ipv6AddressGenerator::Init (network, prefix, base);
}

void
Ipv6AddressHelperCustom::SetBase (
  const Ipv6Address network,
  const Ipv6Prefix prefix,
  const Ipv6Address base)
{
  NS_LOG_FUNCTION (this << network << prefix << base);

    Ipv6AddressGenerator::Init (network, Ipv6Prefix (64), base);
}

Ipv6Address
Ipv6AddressHelperCustom::NewAddress (void)
{
  NS_LOG_FUNCTION (this);
//
// The way this is expected to be used is that an address and network number
// are initialized, and then NewAddress() is called repeatedly to allocate and
// get new addresses on a given subnet.  The client will expect that the first
// address she gets back is the one she used to initialize the generator with.
// This implies that this operation is a post-increment.
//

  Ipv6Address addr =  Ipv6AddressGenerator::NextAddress (Ipv6Prefix (64));
//
// The Ipv6AddressGenerator allows us to keep track of the addresses we have
// allocated and will assert if we accidentally generate a duplicate.  This
// avoids some really hard to debug problems.
//
  if (m_checkAddressDuplication)
    Ipv6AddressGenerator::AddAllocated (addr);
  return addr;
}

Ipv6Address
Ipv6AddressHelperCustom::NewNetwork (void)
{
  NS_LOG_FUNCTION (this);
  Ipv6AddressGenerator::NextNetwork (Ipv6Prefix (64));
}

Ipv6InterfaceContainer
Ipv6AddressHelperCustom::Assign (const NetDeviceContainer &c)
{
  NS_LOG_FUNCTION (this);
  Ipv6InterfaceContainer retval;
  for (uint32_t i = 0; i < c.GetN (); ++i) {
      Ptr<NetDevice> device = c.Get (i);

      Ptr<Node> node = device->GetNode ();
      NS_ASSERT_MSG (node, "Ipv6AddressHelper::Allocate (): Bad node");

      Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
      NS_ASSERT_MSG (ipv6, "Ipv6AddressHelper::Allocate (): Bad ipv6");

      int32_t ifIndex = 0;

      ifIndex = ipv6->GetInterfaceForDevice (device);
      if (ifIndex == -1)
        {
          ifIndex = ipv6->AddInterface (device);
        }
      NS_ASSERT_MSG (ifIndex >= 0, "Ipv6AddressHelper::Allocate (): "
                     "Interface index not found");

       Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (NewAddress (), Ipv6Prefix (64));
       ipv6->SetMetric (ifIndex, 1);
       ipv6->AddAddress (ifIndex, ipv6Addr);
       ipv6->SetUp (ifIndex);

       retval.Add (ipv6, ifIndex);

       // Install the default traffic control configuration if the traffic
       // control layer has been aggregated, if this is not
       // a loopback interface, and there is no queue disc installed already
       Ptr<TrafficControlLayer> tc = node->GetObject<TrafficControlLayer> ();
       if (tc && DynamicCast<LoopbackNetDevice> (device) == 0 && tc->GetRootQueueDiscOnDevice (device) == 0)
         {
           NS_LOG_LOGIC ("Installing default traffic control configuration");
           TrafficControlHelper tcHelper = TrafficControlHelper::Default ();
           tcHelper.Install (device);
         }
    }
  return retval;
}

} // namespace ns3
