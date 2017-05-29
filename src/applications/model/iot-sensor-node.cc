/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 *
 * Author: Saptarshi Gan <sapedu11@gmail.com>
 */

 #include <algorithm>
 #include "ns3/application.h"
 #include "ns3/event-id.h"
 #include "ns3/ptr.h"
 #include "ns3/traced-callback.h"
 #include "ns3/address.h"
 #include "ns3/iot-sensor-node.h"
 #include "ns3/internet-module.h"
 #include "ns3/internet-apps-module.h"
 #include "blockchain.h"
 #include "ns3/boolean.h"
 #include "ns3/address-utils.h"
 #include <crypto++/rsa.h>
 #include <crypto++/osrng.h>
 #include <crypto++/files.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IotSensorNode");

NS_OBJECT_ENSURE_REGISTERED (IotSensorNode);

TypeId
IotSensorNode::GetTypeId (void)
{
        static TypeId tid = TypeId ("ns3::IotSensorNode")
                            .SetParent<Application> ()
                            .SetGroupName("Applications")
                            .AddConstructor<IotSensorNode> ()
                            .AddAttribute ("Local",
                                           "The Address on which to Bind the rx socket.",
                                           AddressValue (),
                                           MakeAddressAccessor (&IotSensorNode::m_local),
                                           MakeAddressChecker ())
                            .AddAttribute ("Protocol",
                                           "The type id of the protocol to use for the rx socket.",
                                           TypeIdValue (UdpSocketFactory::GetTypeId ()),
                                           MakeTypeIdAccessor (&IotSensorNode::m_tid),
                                           MakeTypeIdChecker ())
                            .AddTraceSource ("Rx",
                                             "A packet has been received",
                                             MakeTraceSourceAccessor (&IotSensorNode::m_rxTrace),
                                             "ns3::Packet::AddressTracedCallback")
        ;
        return tid;
}
IotSensorNode::IotSensorNode (void) : m_commPort (5555), m_secondsPerMin(60)
{
        NS_LOG_FUNCTION (this);
        m_socket = 0;
        m_gatewayNodeId = 0;
        m_gatewayAddress = 0;
        m_numberOfPeers = m_peersAddresses.size();
        CryptoPP::AutoSeededRandomPool prng;
        CryptoPP::InvertibleRSAFunction params;
        params.GenerateRandomWithKeySize(prng, 2048);

        CryptoPP::RSA::PrivateKey privateKey(params);
        CryptoPP::RSA::PublicKey publicKey(params);
        m_privateKey = privateKey;
}

IotSensorNode::IotSensorNode (CryptoPP::RSA::PublicKey gatewayKey, CryptoPP::RSA::PrivateKey privKey) : m_commPort (5555), m_secondsPerMin(60)
{
        NS_LOG_FUNCTION (this);
        m_socket = 0;
        m_gatewayNodeId = 0;
        m_gatewayAddress = 0;
        m_privateKey = privKey;
        m_gatewayPublicKey = gatewayKey;
        m_numberOfPeers = m_peersAddresses.size();
}

IotSensorNode::IotSensorNode (CryptoPP::RSA::PublicKey gatewayKey) : m_commPort (5555), m_secondsPerMin(60)
{
        NS_LOG_FUNCTION (this);
        m_socket = 0;
        m_gatewayNodeId = 0;
        m_gatewayAddress = 0;
        m_gatewayPublicKey = gatewayKey;
        m_numberOfPeers = m_peersAddresses.size();
        CryptoPP::AutoSeededRandomPool prng;
        CryptoPP::InvertibleRSAFunction params;
        params.GenerateRandomWithKeySize(prng, 2048);

        CryptoPP::RSA::PrivateKey privateKey(params);
        CryptoPP::RSA::PublicKey publicKey(params);
        m_privateKey = privateKey;
}

IotSensorNode::~IotSensorNode ()
{
        NS_LOG_FUNCTION (this);
}

Ptr<Socket>
IotSensorNode::GetListeningSocket (void) const
{
        NS_LOG_FUNCTION (this);
        return m_socket;
}

std::vector<Ipv4Address>
IotSensorNode::GetPeersAddresses (void) const
{
        NS_LOG_FUNCTION (this);
        return m_peersAddresses;
}

void
IotSensorNode::SetPeersAddresses (const std::vector<Ipv4Address> &peers)
{
  NS_LOG_FUNCTION (this);
  m_peersAddresses = peers;
  m_numberOfPeers = m_peersAddresses.size();
}

void
IotSensorNode::AddPeer (Ipv4Address newPeer)
{
  NS_LOG_FUNCTION (this);
  m_peersAddresses.push_back(newPeer);
  m_numberOfPeers = m_peersAddresses.size();
}

void
IotSensorNode::SetPeersDownloadSpeeds (const std::map<Ipv4Address, double> &peersDownloadSpeeds)
{
        NS_LOG_FUNCTION (this);
        m_peersDownloadSpeeds = peersDownloadSpeeds;
}

void
IotSensorNode::SetPeersUploadSpeeds (const std::map<Ipv4Address, double> &peersUploadSpeeds)
{
        NS_LOG_FUNCTION (this);
        m_peersUploadSpeeds = peersUploadSpeeds;
}

void
IotSensorNode::SetNodeInternetSpeeds (const nodeInternetSpeeds &internetSpeeds)
{
        NS_LOG_FUNCTION (this);

        m_downloadSpeed = internetSpeeds.downloadSpeed * 1000000 / 8;
        m_uploadSpeed = internetSpeeds.uploadSpeed * 1000000 / 8;
}

void
IotSensorNode::SetNodeStats (nodeStatistics *nodeStats)
{
        NS_LOG_FUNCTION (this);
        m_nodeStats = nodeStats;
}

void
IotSensorNode::SetProtocolType (enum ProtocolType protocolType)
{
        NS_LOG_FUNCTION (this);
        m_protocolType = protocolType;
}

void
IotSensorNode::DoDispose (void)
{
        NS_LOG_FUNCTION (this);
        m_socket = 0;

        // chain up
        Application::DoDispose ();
}

// Application Methods
void
IotSensorNode::StartApplication ()    // Called at time specified by Start
{
        NS_LOG_FUNCTION (this);

        srand(time(NULL) + GetNode()->GetId());
        NS_LOG_INFO ("Node " << GetNode()->GetId() << ": download speed = " << m_downloadSpeed << " B/s");
        NS_LOG_INFO ("Node " << GetNode()->GetId() << ": upload speed = " << m_uploadSpeed << " B/s");
        NS_LOG_INFO ("Node " << GetNode()->GetId() << ": m_numberOfPeers = " << m_numberOfPeers);
        NS_LOG_WARN ("Node " << GetNode()->GetId() << ": m_protocolType = " << getProtocolType(m_protocolType));

        NS_LOG_INFO ("Node " << GetNode()->GetId() << ": Gateway node = " << m_gatewayNodeId);
        NS_LOG_INFO ("Node " << GetNode()->GetId() << ": My peers are");

        for (auto it = m_peersAddresses.begin(); it != m_peersAddresses.end(); it++)
                NS_LOG_INFO("\t" << *it);

        double currentMax = 0;

        if (!m_socket)
        {
                m_socket = Socket::CreateSocket (GetNode (), m_tid);
                m_socket->Bind (m_local);
                m_socket->Listen ();
                m_socket->ShutdownSend ();
                if (addressUtils::IsMulticast (m_local))
                {
                        Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
                        if (udpSocket)
                        {
                                // equivalent to setsockopt (MCAST_JOIN_GROUP)
                                udpSocket->MulticastJoinGroup (0, m_local);
                        }
                        else
                        {
                                NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
                        }
                }
        }

        m_socket->SetRecvCallback (MakeCallback (&IotSensorNode::HandleRead, this));
        m_socket->SetAcceptCallback (
                MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
                MakeCallback (&IotSensorNode::HandleAccept, this));
        m_socket->SetCloseCallbacks (
                MakeCallback (&IotSensorNode::HandlePeerClose, this),
                MakeCallback (&IotSensorNode::HandlePeerError, this));

        NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": Before creating sockets");
        for (std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
        {
                m_peersSockets[*i] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
                m_peersSockets[*i]->Connect (InetSocketAddress (*i, m_commPort));
        }
        NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": After creating sockets");
        m_nodeStats->nodeId = GetNode ()->GetId ();
        m_nodeStats->connections = m_peersAddresses.size();
}

void
IotSensorNode::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  for (std::vector<Ipv4Address>::iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i) //close the outgoing sockets
  {
    m_peersSockets[*i]->Close ();
  }


  if (m_socket)
  {
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }

  NS_LOG_WARN ("\n\nIoT Sensor Node " << GetNode ()->GetId () << ":");

}

//Verify sign for message
bool
IotSensorNode::checkSign (std::string message, std::string sign)
{
    return true;
}

bool
IotSensorNode::checkSign (std::string message, std::string sign, Ipv4Address sender)
{
    std::string publicKey = m_publicKeys[sender];
    return true;
}

//Decrypt a message
std::string
IotSensorNode::decrypt (std::string message, Ipv4Address sender)
{
    std::string publicKey = m_publicKeys[sender];
    return message;
}

void
IotSensorNode::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)))
  {
    if (packet->GetSize () == 0)
    { //EOF
       break;
    }

    if (InetSocketAddress::IsMatchingType (from))
    {
      /**
       * We may receive more than one packets simultaneously on the socket,
       * so we have to parse each one of them.
       */
       std::string delimiter = "#";
       std::string parsedPacket;
       size_t pos = 0;
       char *packetInfo = new char[packet->GetSize () + 1];
       std::ostringstream totalStream;

       packet->CopyData (reinterpret_cast<uint8_t*>(packetInfo), packet->GetSize ());
       packetInfo[packet->GetSize ()] = '\0'; // ensure that it is null terminated to avoid bugs

       /**
        * Add the buffered data to complete the packet
        */
       totalStream << m_bufferedData[from] << packetInfo;
       std::string totalReceivedData(totalStream.str());
       NS_LOG_INFO("Node " << GetNode ()->GetId () << " Total Received Data: " << totalReceivedData);

       while ((pos = totalReceivedData.find(delimiter)) != std::string::npos)
       {
         parsedPacket = totalReceivedData.substr(0, pos);
         NS_LOG_INFO("Node " << GetNode ()->GetId () << " Parsed Packet: " << parsedPacket);

         rapidjson::Document d;
         d.Parse(parsedPacket.c_str());

         if(!d.IsObject())
         {
           NS_LOG_WARN("The parsed packet is corrupted");
           totalReceivedData.erase(0, pos + delimiter.length());
           continue;
         }

         rapidjson::StringBuffer buffer;
         rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
         d.Accept(writer);

         NS_LOG_INFO ("At time "  << Simulator::Now ().GetSeconds ()
                       << "s iot node " << GetNode ()->GetId () << " received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " with info = " << buffer.GetString());

         switch (d["message"].GetInt())
         {
           case RECEIVE_PUBLIC_KEY:
           {
             NS_LOG_INFO("RECEIVE_PUBLIC_KEY");
             std::string msgDelimiter = "/";
             std::string parsedBody = d["key"].GetString();
             size_t keyPos = parsedBody.find(msgDelimiter);
             Ipv4Address nodeIp4Address = InetSocketAddress::ConvertFrom(from).GetIpv4 ();

             std::string publicKey = parsedBody.substr(0,keyPos).c_str();
             std::string signature = parsedBody.substr(keyPos+1, parsedBody.size()).c_str();
             if(!checkSign(publicKey,signature,m_gatewayAddress)){
               NS_LOG_WARN("Key not received from nodes' Gateway " << m_gatewayNodeId << " Message sent by " << nodeIp4Address);
               break;
             }
             m_publicKeys[nodeIp4Address]=publicKey;
           }
           case RECEIVE_MESSAGE:
           {
             NS_LOG_INFO("RECEIVE_MESSAGE");
             std::string msgDelimiter = "/";
             std::string parsedBody = d["body"].GetString();
             size_t msgPos = parsedBody.find(msgDelimiter);
             Ipv4Address nodeIp4Address = InetSocketAddress::ConvertFrom(from).GetIpv4 ();

             std::string msgBody = parsedBody.substr(0,msgPos).c_str();
             std::string signature = parsedBody.substr(msgPos+1, parsedBody.size()).c_str();

             if(m_publicKeys.find(nodeIp4Address)==m_publicKeys.end())
             {
               NS_LOG_INFO("PublicKey not present.Requesting Public Key");
               //Request Public Key
             }
             if(!checkSign(msgBody,signature,nodeIp4Address)){
               NS_LOG_WARN("Message not received from claimed sender "  << nodeIp4Address);
               break;
             }

             std::string decryptedMessage = decrypt(msgBody,nodeIp4Address);
             NS_LOG_INFO("Received message " << decryptedMessage << " from node " << nodeIp4Address);
             m_messages[nodeIp4Address].push_back(decryptedMessage);
           }
           default:
             NS_LOG_INFO ("Default");
             break;
         }

         totalReceivedData.erase(0, pos + delimiter.length());
       }

       /**
       * Buffer the remaining data
       */

       m_bufferedData[from] = totalReceivedData;
       delete[] packetInfo;
    }
    else if (Inet6SocketAddress::IsMatchingType (from))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s iot node " << GetNode ()->GetId () << " received "
                   <<  packet->GetSize () << " bytes from "
                   << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                   << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ());
    }
    m_rxTrace (packet, from);
  }
}

void
IotSensorNode::SendMessage (enum Messages receivedMessage, enum Messages responseMessage, rapidjson::Document &d, Ptr<Socket> outgoingSocket)
{
  NS_LOG_FUNCTION (this);

  const uint8_t delimiter[] = "#";

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  d["message"].SetInt(responseMessage);
  d.Accept(writer);
  NS_LOG_INFO("Node " << GetNode ()->GetId () << " sent a " << getMessageName(responseMessage) << " message: " << buffer.GetString());

  outgoingSocket->Send (reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
  outgoingSocket->Send (delimiter, 1, 0);
}

void
IotSensorNode::SendMessage (enum Messages receivedMessage, enum Messages responseMessage, rapidjson::Document &d, Address &outgoingAddress)
{
  NS_LOG_FUNCTION (this);

  const uint8_t delimiter[] = "#";

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  d["message"].SetInt(responseMessage);
  d.Accept(writer);
  NS_LOG_INFO("Node " << GetNode ()->GetId () << " sent a " << getMessageName(responseMessage) << " message: " << buffer.GetString());

  Ipv4Address outgoingIpv4Address = InetSocketAddress::ConvertFrom(outgoingAddress).GetIpv4 ();
  std::map<Ipv4Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv4Address);

  if (it == m_peersSockets.end())
  {
    m_peersSockets[outgoingIpv4Address] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    m_peersSockets[outgoingIpv4Address]->Connect (InetSocketAddress (outgoingIpv4Address, m_commPort));
  }
  Ptr<Socket> outgoingSocket = m_peersSockets[outgoingIpv4Address];
  outgoingSocket->Send (reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
  outgoingSocket->Send (delimiter, 1, 0);
}

void
IotSensorNode::SendMessage (enum Messages receivedMessage, enum Messages responseMessage, std::string packet, Address &outgoingAddress)
{
  NS_LOG_FUNCTION (this);

  const uint8_t delimiter[] = "#";

  rapidjson::Document d;
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  d.Parse(packet.c_str());
  d["message"].SetInt(responseMessage);
  d.Accept(writer);
  NS_LOG_INFO("Node " <<  GetNode ()->GetId () << " sent a " << getMessageName(responseMessage) << " message: " << buffer.GetString());

  Ipv4Address outgoingIpv4Address = InetSocketAddress::ConvertFrom(outgoingAddress).GetIpv4 ();
  std::map<Ipv4Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv4Address);

  if (it == m_peersSockets.end())
  {
    m_peersSockets[outgoingIpv4Address] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    m_peersSockets[outgoingIpv4Address]->Connect (InetSocketAddress (outgoingIpv4Address, m_commPort));
  }
  Ptr<Socket> outgoingSocket = m_peersSockets[outgoingIpv4Address];
  outgoingSocket->Send (reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
  outgoingSocket->Send (delimiter, 1, 0);
}

void
IotSensorNode::SendMessage (enum Messages receivedMessage, enum Messages responseMessage, std::string packet, Ipv4Address &outgoingIpv4Address)
{
  NS_LOG_FUNCTION (this);

  const uint8_t delimiter[] = "#";

  rapidjson::Document d;
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  d.Parse(packet.c_str());
  d["message"].SetInt(responseMessage);
  d.Accept(writer);
  NS_LOG_INFO("Node " << GetNode ()->GetId () << " sent a " << getMessageName(responseMessage) << " message: " << buffer.GetString());

  std::map<Ipv4Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv4Address);

  if (it == m_peersSockets.end())
  {
    m_peersSockets[outgoingIpv4Address] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    m_peersSockets[outgoingIpv4Address]->Connect (InetSocketAddress (outgoingIpv4Address, m_commPort));
  }
  Ptr<Socket> outgoingSocket = m_peersSockets[outgoingIpv4Address];
  outgoingSocket->Send (reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
  outgoingSocket->Send (delimiter, 1, 0);
}

void
IotSensorNode::SendMessage (enum Messages receivedMessage, enum Messages responseMessage, std::string packet, Ptr<Socket> outgoingSocket)
{
  NS_LOG_FUNCTION (this);

  const uint8_t delimiter[] = "#";

  rapidjson::Document d;
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  d.Parse(packet.c_str());
  d["message"].SetInt(responseMessage);
  d.Accept(writer);
  NS_LOG_INFO("Node " << GetNode ()->GetId () << " sent a " << getMessageName(responseMessage) << " message: " << buffer.GetString());

  outgoingSocket->Send (reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
  outgoingSocket->Send (delimiter, 1, 0);
}

void
IotSensorNode::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void IotSensorNode::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void
IotSensorNode::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&IotSensorNode::HandleRead, this));
}

} //namespace ns3
