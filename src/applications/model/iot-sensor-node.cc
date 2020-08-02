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
 #include <cryptopp/rsa.h>
 #include <cryptopp/aes.h>
 #include <cryptopp/osrng.h>
 #include <cryptopp/files.h>
 #include <cryptopp/filters.h>
 #include <cryptopp/modes.h>

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
        m_gatewayAddress = Ipv6Address ("::1");
        m_numberOfPeers = m_peersAddresses.size();
        CryptoPP::AutoSeededRandomPool m_prng;
        CryptoPP::InvertibleRSAFunction params;
        params.GenerateRandomWithKeySize(m_prng, 2048);

        CryptoPP::RSA::PrivateKey privateKey(params);
        CryptoPP::RSA::PublicKey publicKey(params);
        m_privateKey = privateKey;
        m_publicKey = publicKey;
}

IotSensorNode::IotSensorNode (CryptoPP::RSA::PublicKey gatewayKey, CryptoPP::RSA::PrivateKey privKey) : m_commPort (5555), m_secondsPerMin(60)
{
        NS_LOG_FUNCTION (this);
        m_socket = 0;
        m_gatewayNodeId = 0;
        m_gatewayAddress = Ipv6Address ("::1");
        m_privateKey = privKey;
        m_gatewayPublicKey = gatewayKey;
        m_numberOfPeers = m_peersAddresses.size();
}

IotSensorNode::IotSensorNode (CryptoPP::RSA::PublicKey gatewayKey, CryptoPP::RSA::PrivateKey privKey,CryptoPP::RSA::PublicKey pubKey) : m_commPort (5555), m_secondsPerMin(60)
{
        NS_LOG_FUNCTION (this);
        m_socket = 0;
        m_gatewayNodeId = 0;
        m_gatewayAddress = Ipv6Address ("::1");
        m_privateKey = privKey;
        m_publicKey = pubKey;
        m_gatewayPublicKey = gatewayKey;
        m_numberOfPeers = m_peersAddresses.size();
}

IotSensorNode::IotSensorNode (CryptoPP::RSA::PublicKey gatewayKey) : m_commPort (5555), m_secondsPerMin(60)
{
        NS_LOG_FUNCTION (this);
        m_socket = 0;
        m_gatewayNodeId = 0;
        m_gatewayAddress = Ipv6Address ("::1");
        m_gatewayPublicKey = gatewayKey;
        m_numberOfPeers = m_peersAddresses.size();
        CryptoPP::AutoSeededRandomPool m_prng;
        CryptoPP::InvertibleRSAFunction params;
        params.GenerateRandomWithKeySize(m_prng, 2048);

        CryptoPP::RSA::PrivateKey privateKey(params);
        CryptoPP::RSA::PublicKey publicKey(params);
        m_privateKey = privateKey;
        m_publicKey = publicKey;
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

std::vector<Ipv6Address>
IotSensorNode::GetPeersAddresses (void) const
{
        NS_LOG_FUNCTION (this);
        return m_peersAddresses;
}

CryptoPP::RSA::PublicKey
IotSensorNode::GetPublickey (void) const
{
    NS_LOG_FUNCTION (this);
    return m_publicKey;
}

CryptoPP::RSA::PrivateKey
IotSensorNode::GetPrivatekey (void) const
{
    NS_LOG_FUNCTION (this);
    return m_privateKey;
}

void IotSensorNode::SetNewKeys(CryptoPP::RSA::PrivateKey newPrivKey, CryptoPP::RSA::PublicKey newPublicKey)
{
  NS_LOG_FUNCTION (this);
  m_privateKey = newPrivKey;
  m_publicKey = newPublicKey;
}

CryptoPP::RSA::PublicKey
IotSensorNode::GetGatewayPublickey (void) const
{
    NS_LOG_FUNCTION (this);
    return m_gatewayPublicKey;
}

void
IotSensorNode::SetGatewayPublickey (CryptoPP::RSA::PublicKey newGatewayKey)
{
    NS_LOG_FUNCTION (this);
    m_gatewayPublicKey = newGatewayKey;
}

void
IotSensorNode::SetPeersAddresses (const std::vector<Ipv6Address> &peers)
{
  NS_LOG_FUNCTION (this);
  m_peersAddresses = peers;
  m_numberOfPeers = m_peersAddresses.size();
}

void
IotSensorNode::AddPeer (Ipv6Address newPeer)
{
  NS_LOG_FUNCTION (this);
  m_peersAddresses.push_back(newPeer);
  m_numberOfPeers = m_peersAddresses.size();
}

void
IotSensorNode::SetPeersDownloadSpeeds (const std::map<Ipv6Address, double> &peersDownloadSpeeds)
{
        NS_LOG_FUNCTION (this);
        m_peersDownloadSpeeds = peersDownloadSpeeds;
}

void
IotSensorNode::SetPeersUploadSpeeds (const std::map<Ipv6Address, double> &peersUploadSpeeds)
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
        for (std::vector<Ipv6Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
        {
                m_peersSockets[*i] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
                m_peersSockets[*i]->Connect (Inet6SocketAddress (*i, m_commPort));
        }
        NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": After creating sockets");
        m_nodeStats->nodeId = GetNode ()->GetId ();
        m_nodeStats->connections = m_peersAddresses.size();
}

void
IotSensorNode::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  for (std::vector<Ipv6Address>::iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i) //close the outgoing sockets
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
IotSensorNode::checkSign (std::string message, std::string signature)
{
    CryptoPP::RSASSA_PKCS1v15_SHA_Verifier verifier(m_publicKey);
    try{
      CryptoPP::StringSource ss2(message+signature, true, new CryptoPP::SignatureVerificationFilter(verifier, NULL, CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION) // SignatureVerificationFilter
      ); // StringSource
    }
    catch(...){
      return false;
    }
    return true;
}

bool
IotSensorNode::checkSign (std::string message, std::string signature, Ipv6Address sender)
{
    CryptoPP::RSA::PublicKey publicKey = m_publicKeys[sender];
    CryptoPP::RSASSA_PKCS1v15_SHA_Verifier verifier(publicKey);
    try{
      CryptoPP::StringSource ss2(message+signature, true, new CryptoPP::SignatureVerificationFilter(verifier, NULL, CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION) // SignatureVerificationFilter
      ); // StringSource
    }
    catch(...){
      return false;
    }
    return true;
}

//Decrypt a message
std::string
IotSensorNode::decrypt (std::string message)
{
    CryptoPP::RSA::PrivateKey privKey = m_privateKey;
    CryptoPP::RSAES_OAEP_SHA_Decryptor d(privKey);
    std::string recovered;
    CryptoPP::StringSource ss2(message, true,
    new CryptoPP::PK_DecryptorFilter(m_prng, d,
        new CryptoPP::StringSink(recovered)
   ) // PK_DecryptorFilter
    ); // StringSource
    return recovered;
}

//Encrypt a message
std::string
IotSensorNode::encrypt(std::string message, CryptoPP::RSA::PublicKey publicKey){
    std::string cipher;
    CryptoPP::RSAES_OAEP_SHA_Encryptor e(publicKey);
    CryptoPP::StringSource ss1(message, true,
    new CryptoPP::PK_EncryptorFilter(m_prng, e,
        new CryptoPP::StringSink(cipher)
       ) // PK_EncryptorFilter
    ); // StringSource
    return cipher;
}

std::string
IotSensorNode::encrypt(std::string message, Ipv6Address receiver,int type){
  if(type==1){
      CryptoPP::RSA::PublicKey publicKey = m_publicKeys[receiver];
      std::string cipher;
      CryptoPP::RSAES_OAEP_SHA_Encryptor e(publicKey);
      CryptoPP::StringSource ss1(message, true,
      new CryptoPP::PK_EncryptorFilter(m_prng, e,
          new CryptoPP::StringSink(cipher)
       ) // PK_EncryptorFilter
    ); // StringSource
      return cipher;
    }
    else if(type==2){
      std::string cipher;
      byte* aes_key = m_cacheSessionKeys[receiver];
      byte* iv = m_cacheSessionIVs[receiver];
      CryptoPP::AES::Encryption aesEncryption(aes_key, sizeof(aes_key));
      CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, iv );
      // CryptoPP::CFB_Mode<AES>::Encryption cfbEncryption(aes_key, sizeof(aes_key), iv, 1);
      CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( cipher ) );
      stfEncryptor.Put( reinterpret_cast<const unsigned char*>( message.c_str() ), message.length() + 1 );
      stfEncryptor.MessageEnd();
      return cipher;
    }
    else
    return "";
}

std::string
IotSensorNode::encrypt(std::string message, byte* aes_key, byte* iv){
  std::string cipher;
  CryptoPP::AES::Encryption aesEncryption(aes_key, sizeof(aes_key));
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, iv );
  // CryptoPP::CFB_Mode<AES>::Encryption cfbEncryption(aes_key, sizeof(aes_key), iv, 1);
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( cipher ) );
  stfEncryptor.Put( reinterpret_cast<const unsigned char*>( message.c_str() ), message.length() + 1 );
  stfEncryptor.MessageEnd();
  return cipher;
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

    if (Inet6SocketAddress::IsMatchingType (from))
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
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " with info = " << buffer.GetString());

         switch (d["message"].GetInt())
         {
           case RECEIVE_PUBLIC_KEY:
           {
             NS_LOG_INFO("RECEIVE_PUBLIC_KEY");
             std::string msgDelimiter = "/";
             std::string parsedBody = d["key"].GetString();
             size_t keyPos = parsedBody.find(msgDelimiter);
             Ipv6Address nodeIp4Address = Inet6SocketAddress::ConvertFrom(from).GetIpv6 ();

             std::string publicKeyStr = parsedBody.substr(0,keyPos).c_str();
             std::string signature = parsedBody.substr(keyPos+1, parsedBody.size()).c_str();
             if(!checkSign(publicKeyStr,signature,m_gatewayAddress)){
               NS_LOG_WARN("Key not received from nodes' Gateway " << m_gatewayNodeId << " Message sent by " << nodeIp4Address);
               break;
             }
             CryptoPP::RSA::PublicKey publicKey;
             CryptoPP::StringSource publicKeySource(publicKeyStr,true);
             publicKey.BERDecode(publicKeySource);
             m_publicKeys[nodeIp4Address]=publicKey;
           }
           case RECEIVE_MESSAGE:
           {
             NS_LOG_INFO("RECEIVE_MESSAGE");
             std::string msgDelimiter = "/";
             std::string parsedBody = d["body"].GetString();
             size_t msgPos = parsedBody.find(msgDelimiter);
             Ipv6Address nodeIp4Address = Inet6SocketAddress::ConvertFrom(from).GetIpv6 ();

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

             std::string decryptedMessage = decrypt(msgBody);
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

  Ipv6Address outgoingIpv6Address = Inet6SocketAddress::ConvertFrom(outgoingAddress).GetIpv6 ();
  std::map<Ipv6Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv6Address);

  if (it == m_peersSockets.end())
  {
    m_peersSockets[outgoingIpv6Address] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    m_peersSockets[outgoingIpv6Address]->Connect (Inet6SocketAddress (outgoingIpv6Address, m_commPort));
  }
  Ptr<Socket> outgoingSocket = m_peersSockets[outgoingIpv6Address];
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

  Ipv6Address outgoingIpv6Address = Inet6SocketAddress::ConvertFrom(outgoingAddress).GetIpv6 ();
  std::map<Ipv6Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv6Address);

  if (it == m_peersSockets.end())
  {
    m_peersSockets[outgoingIpv6Address] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    m_peersSockets[outgoingIpv6Address]->Connect (Inet6SocketAddress (outgoingIpv6Address, m_commPort));
  }
  Ptr<Socket> outgoingSocket = m_peersSockets[outgoingIpv6Address];
  outgoingSocket->Send (reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
  outgoingSocket->Send (delimiter, 1, 0);
}

void
IotSensorNode::SendMessage (enum Messages receivedMessage, enum Messages responseMessage, std::string packet, Ipv6Address &outgoingIpv6Address)
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

  std::map<Ipv6Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv6Address);

  if (it == m_peersSockets.end())
  {
    m_peersSockets[outgoingIpv6Address] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    m_peersSockets[outgoingIpv6Address]->Connect (Inet6SocketAddress (outgoingIpv6Address, m_commPort));
  }
  Ptr<Socket> outgoingSocket = m_peersSockets[outgoingIpv6Address];
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
