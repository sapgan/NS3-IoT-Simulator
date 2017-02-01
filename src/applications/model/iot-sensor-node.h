#ifndef IOT_SENSOR_NODE_H
#define IOT_SENSOR_NODE_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/sixlowpan-module.h"
#include <random>

namespace ns3 {

class Address;
class Socket;
class Packet;

class IotSensorNode : public Application
{
public:
	/**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  IotSensorNode();

  virtual ~IotSensorNode ();

  protected:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  virtual void DoDispose (void);
};



} // namespace ns3

#endif /* IOT_SENSOR_NODE_H */