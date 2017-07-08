/**
 * This file contains the definitions of the functions declared in blockchain-node.h
 */

#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "blockchain-node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BlockchainNode");

NS_OBJECT_ENSURE_REGISTERED (BlockchainNode);

TypeId
BlockchainNode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BlockchainNode")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BlockchainNode> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&BlockchainNode::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&BlockchainNode::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("BlockTorrent",
                   "Enable the BlockTorrent protocol",
                   BooleanValue (false),
                   MakeBooleanAccessor (&BlockchainNode::m_blockTorrent),
                   MakeBooleanChecker ())
    .AddAttribute ("SPV",
                   "Enable SPV Mechanism",
                   BooleanValue (false),
                   MakeBooleanAccessor (&BlockchainNode::m_spv),
                   MakeBooleanChecker ())
    .AddAttribute ("InvTimeoutMinutes",
				   "The timeout of inv messages in minutes",
                   TimeValue (Minutes (20)),
                   MakeTimeAccessor (&BlockchainNode::m_invTimeoutMinutes),
                   MakeTimeChecker())
    .AddAttribute ("ChunkSize",
				   "The fixed size of the block chunk",
                   UintegerValue (100000),
                   MakeUintegerAccessor (&BlockchainNode::m_chunkSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&BlockchainNode::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
  ;
  return tid;
}

BlockchainNode::BlockchainNode (void) : m_bitcoinPort (8333), m_secondsPerMin(60), m_isMiner (false), m_countBytes (4), m_bitcoinMessageHeader (90),
                                  m_inventorySizeBytes (36), m_getHeadersSizeBytes (72), m_headersSizeBytes (81), m_blockHeadersSizeBytes (81),
                                  m_averageTransactionSize (522.4), m_transactionIndexSize (2)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_meanBlockReceiveTime = 0;
  m_previousBlockReceiveTime = 0;
  m_meanBlockPropagationTime = 0;
  m_meanBlockSize = 0;
  m_numberOfPeers = m_peersAddresses.size();

}

BlockchainNode::~BlockchainNode(void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<Socket>
BlockchainNode::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}


std::vector<Ipv6Address>
BlockchainNode::GetPeersAddresses (void) const
{
  NS_LOG_FUNCTION (this);
  return m_peersAddresses;
}


void
BlockchainNode::SetPeersAddresses (const std::vector<Ipv6Address> &peers)
{
  NS_LOG_FUNCTION (this);
  m_peersAddresses = peers;
  m_numberOfPeers = m_peersAddresses.size();
}


void
BlockchainNode::SetPeersDownloadSpeeds (const std::map<Ipv6Address, double> &peersDownloadSpeeds)
{
  NS_LOG_FUNCTION (this);
  m_peersDownloadSpeeds = peersDownloadSpeeds;
}


void
BlockchainNode::SetPeersUploadSpeeds (const std::map<Ipv6Address, double> &peersUploadSpeeds)
{
  NS_LOG_FUNCTION (this);
  m_peersUploadSpeeds = peersUploadSpeeds;
}

void
BlockchainNode::SetNodeInternetSpeeds (const nodeInternetSpeeds &internetSpeeds)
{
  NS_LOG_FUNCTION (this);

  m_downloadSpeed = internetSpeeds.downloadSpeed * 1000000 / 8 ;
  m_uploadSpeed = internetSpeeds.uploadSpeed * 1000000 / 8 ;
}


void
BlockchainNode::SetNodeStats (nodeStatistics *nodeStats)
{
  NS_LOG_FUNCTION (this);
  m_nodeStats = nodeStats;
}

void
BlockchainNode::SetProtocolType (enum ProtocolType protocolType)
{
  NS_LOG_FUNCTION (this);
  m_protocolType = protocolType;
}

void
BlockchainNode::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

  // chain up
  Application::DoDispose ();
}


// Application Methods
void
BlockchainNode::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already

  srand(time(NULL) + GetNode()->GetId());
  NS_LOG_INFO ("Node " << GetNode()->GetId() << ": download speed = " << m_downloadSpeed << " B/s");
  NS_LOG_INFO ("Node " << GetNode()->GetId() << ": upload speed = " << m_uploadSpeed << " B/s");
  NS_LOG_INFO ("Node " << GetNode()->GetId() << ": m_numberOfPeers = " << m_numberOfPeers);
  NS_LOG_INFO ("Node " << GetNode()->GetId() << ": m_invTimeoutMinutes = " << m_invTimeoutMinutes.GetMinutes() << "mins");
  NS_LOG_WARN ("Node " << GetNode()->GetId() << ": m_protocolType = " << getProtocolType(m_protocolType));
  NS_LOG_WARN ("Node " << GetNode()->GetId() << ": m_blockTorrent = " << m_blockTorrent);
  NS_LOG_WARN ("Node " << GetNode()->GetId() << ": m_chunkSize = " << m_chunkSize << " Bytes");

  NS_LOG_INFO ("Node " << GetNode()->GetId() << ": My peers are");


  for (auto it = m_peersAddresses.begin(); it != m_peersAddresses.end(); it++)
    NS_LOG_INFO("\t" << *it);

  double currentMax = 0;

  for(auto it = m_peersDownloadSpeeds.begin(); it != m_peersDownloadSpeeds.end(); ++it )
  {
    //std::cout << "Node " << GetNode()->GetId() << ": peer " << it->first << "download speed = " << it->second << " Mbps" << std::endl;
  }

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


  m_socket->SetRecvCallback (MakeCallback (&BlockchainNode::HandleRead, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&BlockchainNode::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&BlockchainNode::HandlePeerClose, this),
    MakeCallback (&BlockchainNode::HandlePeerError, this));


  NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": Before creating sockets");
  for (std::vector<Ipv6Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
  {
    m_peersSockets[*i] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    m_peersSockets[*i]->Connect (Inet6SocketAddress (*i, m_bitcoinPort));
  }
  NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": After creating sockets");

  // m_nodeStats->nodeId = GetNode ()->GetId ();
  // m_nodeStats->meanBlockReceiveTime = 0;
  // m_nodeStats->meanBlockPropagationTime = 0;
  // m_nodeStats->meanBlockSize = 0;
  // m_nodeStats->totalBlocks = 0;
  // m_nodeStats->staleBlocks = 0;
  // m_nodeStats->miner = 0;
  // m_nodeStats->minerGeneratedBlocks = 0;
  // m_nodeStats->minerAverageBlockGenInterval = 0;
  // m_nodeStats->minerAverageBlockSize = 0;
  // m_nodeStats->hashRate = 0;
  // m_nodeStats->attackSuccess = 0;
  // m_nodeStats->invReceivedBytes = 0;
  // m_nodeStats->invSentBytes = 0;
  // m_nodeStats->getHeadersReceivedBytes = 0;
  // m_nodeStats->getHeadersSentBytes = 0;
  // m_nodeStats->headersReceivedBytes = 0;
  // m_nodeStats->headersSentBytes = 0;
  // m_nodeStats->getDataReceivedBytes = 0;
  // m_nodeStats->getDataSentBytes = 0;
  // m_nodeStats->blockReceivedBytes = 0;
  // m_nodeStats->blockSentBytes = 0;
  // m_nodeStats->extInvReceivedBytes = 0;
  // m_nodeStats->extInvSentBytes = 0;
  // m_nodeStats->extGetHeadersReceivedBytes = 0;
  // m_nodeStats->extGetHeadersSentBytes = 0;
  // m_nodeStats->extHeadersReceivedBytes = 0;
  // m_nodeStats->extHeadersSentBytes = 0;
  // m_nodeStats->extGetDataReceivedBytes = 0;
  // m_nodeStats->extGetDataSentBytes = 0;
  // m_nodeStats->chunkReceivedBytes = 0;
  // m_nodeStats->chunkSentBytes = 0;
  // m_nodeStats->longestFork = 0;
  // m_nodeStats->blocksInForks = 0;
  // m_nodeStats->connections = m_peersAddresses.size();
  // m_nodeStats->blockTimeouts = 0;
  // m_nodeStats->chunkTimeouts = 0;
  // m_nodeStats->minedBlocksInMainChain = 0;
}

void
BlockchainNode::StopApplication ()     // Called at time specified by Stop
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

  NS_LOG_WARN ("\n\nBLOCKCHAIN NODE " << GetNode ()->GetId () << ":");
  NS_LOG_WARN ("Current Top Block is:\n" << *(m_blockchain.GetCurrentTopBlock()));
  NS_LOG_WARN ("Current Blockchain is:\n" << m_blockchain);
  //m_blockchain.PrintOrphans();
  //PrintQueueInv();
  //PrintInvTimeouts();

  NS_LOG_WARN("Mean Block Receive Time = " << m_meanBlockReceiveTime << " or "
               << static_cast<int>(m_meanBlockReceiveTime) / m_secondsPerMin << "min and "
               << m_meanBlockReceiveTime - static_cast<int>(m_meanBlockReceiveTime) / m_secondsPerMin * m_secondsPerMin << "s");
  NS_LOG_WARN("Mean Block Propagation Time = " << m_meanBlockPropagationTime << "s");
  NS_LOG_WARN("Mean Block Size = " << m_meanBlockSize << " Bytes");
  NS_LOG_WARN("Total Blocks = " << m_blockchain.GetTotalBlocks());
  NS_LOG_WARN("Stale Blocks = " << m_blockchain.GetNoStaleBlocks() << " ("
              << 100. * m_blockchain.GetNoStaleBlocks() / m_blockchain.GetTotalBlocks() << "%)");
  NS_LOG_WARN("receivedButNotValidated size = " << m_receivedNotValidated.size());
  NS_LOG_WARN("m_sendBlockTimes size = " << m_sendBlockTimes.size());
  NS_LOG_WARN("m_receiveBlockTimes size = " << m_receiveBlockTimes.size());
  NS_LOG_WARN("longest fork = " << m_blockchain.GetLongestForkSize());
  NS_LOG_WARN("blocks in forks = " << m_blockchain.GetBlocksInForks());

  m_nodeStats->meanBlockReceiveTime = m_meanBlockReceiveTime;
  m_nodeStats->meanBlockPropagationTime = m_meanBlockPropagationTime;
  m_nodeStats->meanBlockSize = m_meanBlockSize;
  m_nodeStats->totalBlocks = m_blockchain.GetTotalBlocks();
  m_nodeStats->staleBlocks = m_blockchain.GetNoStaleBlocks();
  m_nodeStats->longestFork = m_blockchain.GetLongestForkSize();
  m_nodeStats->blocksInForks = m_blockchain.GetBlocksInForks();

}

void
BlockchainNode::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  double newBlockReceiveTime = Simulator::Now ().GetSeconds();

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
                        << "s bitcoin node " << GetNode ()->GetId () << " received "
                        <<  packet->GetSize () << " bytes from "
                        << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                        << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                        << " with info = " << buffer.GetString());

          switch (d["message"].GetInt())
          {
            case INV:
            {
              //NS_LOG_INFO ("INV");
              int j;
              std::vector<std::string>            requestBlocks;
              std::vector<std::string>::iterator  block_it;

              m_nodeStats->invReceivedBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;

              for (j=0; j<d["inv"].Size(); j++)
              {
                std::string   invDelimiter = "/";
                std::string   parsedInv = d["inv"][j].GetString();
                size_t        invPos = parsedInv.find(invDelimiter);
                EventId       timeout;

                int height = atoi(parsedInv.substr(0, invPos).c_str());
                int minerId = atoi(parsedInv.substr(invPos+1, parsedInv.size()).c_str());


                if (m_blockchain.HasBlock(height, minerId) || m_blockchain.IsOrphan(height, minerId) || ReceivedButNotValidated(parsedInv))
                {
                  NS_LOG_INFO("INV: Bitcoin node " << GetNode ()->GetId ()
                              << " has already received the block with height = "
                              << height << " and minerId = " << minerId);
                }
                else
                {
                  NS_LOG_INFO("INV: Bitcoin node " << GetNode ()->GetId ()
                              << " does not have the block with height = "
                              << height << " and minerId = " << minerId);

                  /**
                   * Check if we have already requested the block
                   */

                  if (m_invTimeouts.find(parsedInv) == m_invTimeouts.end())
                  {
                    NS_LOG_INFO("INV: Bitcoin node " << GetNode ()->GetId ()
                                 << " has not requested the block yet");
                    requestBlocks.push_back(parsedInv);
                    timeout = Simulator::Schedule (m_invTimeoutMinutes, &BlockchainNode::InvTimeoutExpired, this, parsedInv);
                    m_invTimeouts[parsedInv] = timeout;
                  }
                  else
                  {
                    NS_LOG_INFO("INV: Bitcoin node " << GetNode ()->GetId ()
                                 << " has already requested the block");
                  }

                  m_queueInv[parsedInv].push_back(from);
                  //PrintQueueInv();
                  //PrintInvTimeouts();
                }
              }

              if (!requestBlocks.empty())
              {
                rapidjson::Value   value;
                rapidjson::Value   array(rapidjson::kArrayType);
                d.RemoveMember("inv");

                for (block_it = requestBlocks.begin(); block_it < requestBlocks.end(); block_it++)
                {
                  value.SetString(block_it->c_str(), block_it->size(), d.GetAllocator());
                  array.PushBack(value, d.GetAllocator());
                }

                d.AddMember("blocks", array, d.GetAllocator());

                SendMessage(INV, GET_HEADERS, d, from);
                SendMessage(INV, GET_DATA, d, from);

              }
              break;
            }
//             case EXT_INV:
//             {
//               //NS_LOG_INFO ("EXT_INV");
//               int j;
//               std::vector<std::string>            requestHeaders;
//               std::vector<std::string>            requestChunks;
//
//               std::vector<std::string>::iterator  block_it;
//
//               m_nodeStats->extInvReceivedBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
//
//               for (j=0; j<d["inv"].Size(); j++)
//               {
//                 std::string   invDelimiter = "/";
//                 std::string   blockHash = d["inv"][j]["hash"].GetString();
//                 int           blockSize = d["inv"][j]["size"].GetInt();
//                 size_t        invPos = blockHash.find(invDelimiter);
//                 EventId       timeout;
//
//                 int height = atoi(blockHash.substr(0, invPos).c_str());
//                 int minerId = atoi(blockHash.substr(invPos+1, blockHash.size()).c_str());
//
//                 m_nodeStats->extInvReceivedBytes += 5;
//                 if (!d["inv"][j]["fullBlock"].GetBool())
//                   m_nodeStats->extInvReceivedBytes += d["inv"][j]["availableChunks"].Size();
//
//                 if (m_blockchain.HasBlock(height, minerId) || m_blockchain.IsOrphan(height, minerId) || ReceivedButNotValidated(blockHash))
//                 {
//                   NS_LOG_INFO("EXT_INV: Bitcoin node " << GetNode ()->GetId ()
//                               << " has already received the block with height = "
//                               << height << " and minerId = " << minerId);
//                 }
//                 else
//                 {
//                   NS_LOG_INFO("EXT_INV: Bitcoin node " << GetNode ()->GetId ()
//                               << " does not have the block with height = "
//                               << height << " and minerId = " << minerId);
//
//                   if (m_queueChunks.find(blockHash) == m_queueChunks.end())
//                   {
//                     NS_LOG_INFO("EXT_INV: Bitcoin node " << GetNode ()->GetId ()
//                                 << " does not have an entry in m_queueChunks");
//                     for (int i = 0; i < ceil(blockSize/static_cast<double>(m_chunkSize)); i++)
//                       m_queueChunks[blockHash].push_back(i);
//                   }
//                   //PrintQueueChunks();
//
//
//                   /**
//                    * Check if we have already requested all the chunks
//                    */
//
//                   if (m_queueChunks[blockHash].size() > 0)
//                   {
//                     NS_LOG_INFO("EXT_INV: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has not requested all the chunks yet");
//                     if (!OnlyHeadersReceived(blockHash))
//                       requestHeaders.push_back(blockHash);
//                     //timeout = Simulator::Schedule (m_invTimeoutMinutes, &BlockchainNode::InvTimeoutExpired, this, blockHash);
//                     //m_invTimeouts[blockHash] = timeout;
//
//
//                     std::vector<int> candidateChunks;
//                     if (d["inv"][j]["fullBlock"].GetBool())
//                     {
//                       for (auto &chunk : m_queueChunks[blockHash])
//                         candidateChunks.push_back(chunk);
//                     }
//                     else
//                     {
//                       for (int k = 0; k < d["inv"][j]["availableChunks"].Size(); k++)
//                       {
//
//                         if (std::find(m_queueChunks[blockHash].begin(), m_queueChunks[blockHash].end(), d["inv"][j]["availableChunks"][k].GetInt()) != m_queueChunks[blockHash].end())
//                           candidateChunks.push_back(d["inv"][j]["availableChunks"][k].GetInt());
//                       }
//                     }
//
// /*                     std::cout << "candidateChunks = ";
//                     for (auto chunk : candidateChunks)
//                       std::cout << chunk << ", ";
//                     std::cout << "\n"; */
//
//                     if (candidateChunks.size() > 0)
//                     {
//                       int randomIndex = rand() % candidateChunks.size();
//                       NS_LOG_INFO("EXT_INV: Bitcoin node " << GetNode ()->GetId ()
//                                   << " will request the chunk with index = " << randomIndex << " and value = " << candidateChunks[randomIndex]);
//                       m_queueChunks[blockHash].erase(std::remove(m_queueChunks[blockHash].begin(),
//                                                                  m_queueChunks[blockHash].end(), candidateChunks[randomIndex]),
//                                                                  m_queueChunks[blockHash].end());
//
//                       std::ostringstream chunk;
//                       chunk << blockHash << "/" << candidateChunks[randomIndex];
//                       requestChunks.push_back(chunk.str());
//
//                       timeout = Simulator::Schedule (Minutes(m_invTimeoutMinutes.GetMinutes() / ceil(blockSize/static_cast<double>(m_chunkSize))),
//                                                      &BlockchainNode::ChunkTimeoutExpired, this, chunk.str());
//
//                       m_chunkTimeouts[chunk.str()] = timeout;
//                       m_queueChunkPeers[blockHash].push_back(from);
//                     }
//                     else
//                     {
//                       NS_LOG_INFO("EXT_INV: Bitcoin node " << GetNode ()->GetId ()
//                                   << " will not request any chunks from this peer, because it has already all the available ones");
//                     }
//
// /*                     PrintQueueChunks();
//                     PrintChunkTimeouts();
//                     PrintQueueChunkPeers();
//                     PrintReceivedChunks(); */
//                   }
//                   else
//                   {
//                     NS_LOG_INFO("EXT_INV: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has already requested all the chunks");
//                   }
//
//                 }
//               }
//
//               d.RemoveMember("inv");
//
//               if (!requestHeaders.empty())
//               {
//                 rapidjson::Value   value;
//                 rapidjson::Value   array(rapidjson::kArrayType);
//
//                 for (block_it = requestHeaders.begin(); block_it < requestHeaders.end(); block_it++)
//                 {
//                   value.SetString(block_it->c_str(), block_it->size(), d.GetAllocator());
//                   array.PushBack(value, d.GetAllocator());
//                 }
//
//                 d.AddMember("blocks", array, d.GetAllocator());
//
//                 SendMessage(EXT_INV, EXT_GET_HEADERS, d, from);
//
//               }
//
//               if (!requestChunks.empty())
//               {
//                 rapidjson::Value   value;
//                 rapidjson::Value   chunkArray(rapidjson::kArrayType);
//                 rapidjson::Value   availableChunks(rapidjson::kArrayType);
//                 rapidjson::Value   chunkInfo(rapidjson::kObjectType);
//
//                 d.RemoveMember("type");
//                 d.RemoveMember("blocks");
//
//                 value.SetString("chunk");
//                 d.AddMember("type", value, d.GetAllocator());
//
//                 for (auto chunk_it = requestChunks.begin(); chunk_it < requestChunks.end(); chunk_it++)
//                 {
//
//                   std::string            invDelimiter = "/";
//                   std::string            chunkHash = *chunk_it;
//                   std::string            chunkHashHelp = chunkHash.substr(0);
//                   std::ostringstream     help;
//                   std::string            blockHash;
//                   size_t                 invPos = chunkHashHelp.find(invDelimiter);
//
//                   int height = atoi(chunkHashHelp.substr(0, invPos).c_str());
//
//                   chunkHashHelp.erase(0, invPos + invDelimiter.length());
//                   invPos = chunkHashHelp.find(invDelimiter);
//                   int minerId = atoi(chunkHashHelp.substr(0, invPos).c_str());
//
//                   chunkHashHelp.erase(0, invPos + invDelimiter.length());
//                   int chunkId = atoi(chunkHashHelp.substr(0).c_str());
//                   help << height << "/" << minerId;
//                   blockHash = help.str();
//
//                   if (m_receivedChunks.find(blockHash) != m_receivedChunks.end())
//                   {
//                     for ( auto k : m_receivedChunks[blockHash])
//                     {
//                       value = k;
//                       availableChunks.PushBack(value, d.GetAllocator());
//                     }
//                   }
//                   chunkInfo.AddMember("availableChunks", availableChunks, d.GetAllocator());
//
//                   value = false;
//                   chunkInfo.AddMember("fullBlock", value, d.GetAllocator());
//
//                   value.SetString(chunk_it->c_str(), chunk_it->size(), d.GetAllocator());
//                   chunkInfo.AddMember("chunk", value, d.GetAllocator());
//
//                   chunkArray.PushBack(chunkInfo, d.GetAllocator());
//                 }
//                 d.AddMember("chunks", chunkArray, d.GetAllocator());
//
//                 SendMessage(EXT_INV, EXT_GET_DATA, d, from);
//
//               }
//               break;
//             }
            case GET_HEADERS:
            {
              int j;
              std::vector<Block>              requestHeaders;
              std::vector<Block>::iterator    block_it;

              m_nodeStats->getHeadersReceivedBytes += m_bitcoinMessageHeader + m_getHeadersSizeBytes;

              for (j=0; j<d["blocks"].Size(); j++)
              {
                std::string   invDelimiter = "/";
                std::string   blockHash = d["blocks"][j].GetString();
                size_t        invPos = blockHash.find(invDelimiter);

                int height = atoi(blockHash.substr(0, invPos).c_str());
                int minerId = atoi(blockHash.substr(invPos+1, blockHash.size()).c_str());

                if (m_blockchain.HasBlock(height, minerId) || m_blockchain.IsOrphan(height, minerId))
                {
                  NS_LOG_INFO("GET_HEADERS: Bitcoin node " << GetNode ()->GetId ()
                              << " has the block with height = "
                              << height << " and minerId = " << minerId);
                  Block newBlock (m_blockchain.ReturnBlock (height, minerId));
                  requestHeaders.push_back(newBlock);
                }
                else if (ReceivedButNotValidated(blockHash))
                {
                  NS_LOG_INFO("GET_HEADERS: Bitcoin node " << GetNode ()->GetId ()
                              << " has received but not yet validated the block with height = "
                              << height << " and minerId = " << minerId);
                  requestHeaders.push_back(m_receivedNotValidated[blockHash]);
                }
                else
                {
                  NS_LOG_INFO("GET_HEADERS: Bitcoin node " << GetNode ()->GetId ()
                              << " does not have the full block with height = "
                              << height << " and minerId = " << minerId);

                }
              }

              if (!requestHeaders.empty())
              {
                rapidjson::Value value;
                rapidjson::Value array(rapidjson::kArrayType);

                d.RemoveMember("blocks");

                for (block_it = requestHeaders.begin(); block_it < requestHeaders.end(); block_it++)
                {
                  rapidjson::Value blockInfo(rapidjson::kObjectType);
                  NS_LOG_INFO ("In requestHeaders " << *block_it);

                  value = block_it->GetBlockHeight ();
                  blockInfo.AddMember("height", value, d.GetAllocator ());

                  value = block_it->GetMinerId ();
                  blockInfo.AddMember("minerId", value, d.GetAllocator ());

                  value = block_it->GetParentBlockMinerId ();
                  blockInfo.AddMember("parentBlockMinerId", value, d.GetAllocator ());

                  value = block_it->GetBlockSizeBytes ();
                  blockInfo.AddMember("size", value, d.GetAllocator ());

                  value = block_it->GetTimeCreated ();
                  blockInfo.AddMember("timeCreated", value, d.GetAllocator ());

                  value = block_it->GetTimeReceived ();
                  blockInfo.AddMember("timeReceived", value, d.GetAllocator ());

                  array.PushBack(blockInfo, d.GetAllocator());
                }

                d.AddMember("blocks", array, d.GetAllocator());

                SendMessage(GET_HEADERS, HEADERS, d, from);
              }
              break;
            }
//             case EXT_GET_HEADERS:
//             {
//               int j;
//               std::vector<Block>              requestHeaders;
//               std::vector<Block>::iterator    block_it;
//
//               m_nodeStats->extGetHeadersReceivedBytes += m_bitcoinMessageHeader + m_getHeadersSizeBytes;
//
//               for (j=0; j<d["blocks"].Size(); j++)
//               {
//                 std::string   invDelimiter = "/";
//                 std::string   blockHash = d["blocks"][j].GetString();
//                 size_t        invPos = blockHash.find(invDelimiter);
//
//                 int height = atoi(blockHash.substr(0, invPos).c_str());
//                 int minerId = atoi(blockHash.substr(invPos+1, blockHash.size()).c_str());
//
//                 if (m_blockchain.HasBlock(height, minerId) || m_blockchain.IsOrphan(height, minerId))
//                 {
//                   NS_LOG_INFO("EXT_GET_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                               << " has the block with height = "
//                               << height << " and minerId = " << minerId);
//                   Block newBlock (m_blockchain.ReturnBlock (height, minerId));
//                   requestHeaders.push_back(newBlock);
//                 }
//                 else if (ReceivedButNotValidated(blockHash))
//                 {
//                   NS_LOG_INFO("EXT_GET_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                   << " has received but not yet validated the block with height = "
//                   << height << " and minerId = " << minerId);
//                   requestHeaders.push_back(m_receivedNotValidated[blockHash]);
//                 }
//                 else if (OnlyHeadersReceived(blockHash))
//                 {
//                   NS_LOG_INFO("EXT_GET_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                   << " has received only the headers of the block with hash = " << blockHash);
//                   requestHeaders.push_back(m_onlyHeadersReceived[blockHash]);
//                 }
//                 else
//                 {
//                   NS_LOG_INFO("EXT_GET_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                   << " has neither the block nor the headers of the block hash = " << blockHash);
//
//                 }
//               }
//
//               if (!requestHeaders.empty())
//               {
//                 rapidjson::Value     value;
//                 rapidjson::Value     array(rapidjson::kArrayType);
//                 rapidjson::Value     chunkArray(rapidjson::kArrayType);
//                 rapidjson::Value     chunkInfo(rapidjson::kObjectType);
//                 std::ostringstream   blockHashHelp;
//                 std::string          blockHash;
//
//                 d.RemoveMember("blocks");
//
//                 for (block_it = requestHeaders.begin(); block_it < requestHeaders.end(); block_it++)
//                 {
//                   NS_LOG_INFO ("In requestHeaders " << *block_it);
//
//                   blockHashHelp << block_it->GetBlockHeight () << "/" << block_it->GetMinerId ();
//                   blockHash = blockHashHelp.str();
//
//                   value = block_it->GetBlockHeight ();
//                   chunkInfo.AddMember("height", value, d.GetAllocator ());
//
//                   value = block_it->GetMinerId ();
//                   chunkInfo.AddMember("minerId", value, d.GetAllocator ());
//
//                   value = block_it->GetParentBlockMinerId ();
//                   chunkInfo.AddMember("parentBlockMinerId", value, d.GetAllocator ());
//
//                   value = block_it->GetBlockSizeBytes ();
//                   chunkInfo.AddMember("size", value, d.GetAllocator ());
//
//                   value = block_it->GetTimeCreated ();
//                   chunkInfo.AddMember("timeCreated", value, d.GetAllocator ());
//
//                   value = block_it->GetTimeReceived ();
//                   chunkInfo.AddMember("timeReceived", value, d.GetAllocator ());
//
//                   if (m_blockchain.HasBlock(block_it->GetBlockHeight (), block_it->GetMinerId ())
//                       || m_blockchain.IsOrphan(block_it->GetBlockHeight (), block_it->GetMinerId ())
//                       || ReceivedButNotValidated(blockHash))
//                   {
//                     value = true;
//                     chunkInfo.AddMember("fullBlock", value, d.GetAllocator ());
//                   }
//                   else if (OnlyHeadersReceived(blockHash))
//                   {
//                     int noChunks = ceil(block_it->GetBlockSizeBytes ()/static_cast<double>(m_chunkSize));
//
//                     if (m_receivedChunks[blockHash].size() == noChunks)
//                     {
//                       value = true;
//                       chunkInfo.AddMember("fullBlock", value, d.GetAllocator ());
//                     }
//                     else
//                     {
//                       value = false;
//                       chunkInfo.AddMember("fullBlock", value, d.GetAllocator ());
//
//                       for (auto &chunk : m_receivedChunks[blockHash])
//                       {
//                         value = chunk;
//                         chunkArray.PushBack(value, d.GetAllocator());
//                       }
//                       chunkInfo.AddMember("availableChunks", chunkArray, d.GetAllocator ());
//                     }
// 				  }
//
//                   array.PushBack(chunkInfo, d.GetAllocator());
//                 }
//
//                 d.AddMember("blocks", array, d.GetAllocator());
//
//                 SendMessage(EXT_GET_HEADERS, EXT_HEADERS, d, from);
//               }
//               break;
//             }
//             case GET_DATA:
//             {
//               NS_LOG_INFO ("GET_DATA");
//
//               int j;
//               int totalBlockMessageSize = 0;
//               std::vector<Block>              requestBlocks;
//               std::vector<Block>::iterator    block_it;
//
//               m_nodeStats->getDataReceivedBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_inventorySizeBytes;
//
//               for (j=0; j<d["blocks"].Size(); j++)
//               {
//                 std::string    invDelimiter = "/";
//                 std::string    parsedInv = d["blocks"][j].GetString();
//                 size_t         invPos = parsedInv.find(invDelimiter);
//
//                 int height = atoi(parsedInv.substr(0, invPos).c_str());
//                 int minerId = atoi(parsedInv.substr(invPos+1, parsedInv.size()).c_str());
//
//                 if (m_blockchain.HasBlock(height, minerId))
//                 {
//                   NS_LOG_INFO("GET_DATA: Bitcoin node " << GetNode ()->GetId ()
//                               << " has already received the block with height = "
//                               << height << " and minerId = " << minerId);
//                   Block newBlock (m_blockchain.ReturnBlock (height, minerId));
//                   requestBlocks.push_back(newBlock);
//                 }
//                 else
//                 {
//                   NS_LOG_INFO("GET_DATA: Bitcoin node " << GetNode ()->GetId ()
//                   << " does not have the block with height = "
//                   << height << " and minerId = " << minerId);
//                 }
//               }
//
//               if (!requestBlocks.empty())
//               {
//                 rapidjson::Value value;
//                 rapidjson::Value array(rapidjson::kArrayType);
//
//
//                 d.RemoveMember("blocks");
//
//                 for (block_it = requestBlocks.begin(); block_it < requestBlocks.end(); block_it++)
//                 {
//                   rapidjson::Value blockInfo(rapidjson::kObjectType);
//                   NS_LOG_INFO ("In requestBlocks " << *block_it);
//
//                   value = block_it->GetBlockHeight ();
//                   blockInfo.AddMember("height", value, d.GetAllocator ());
//
//                   value = block_it->GetMinerId ();
//                   blockInfo.AddMember("minerId", value, d.GetAllocator ());
//
//                   value = block_it->GetParentBlockMinerId ();
//                   blockInfo.AddMember("parentBlockMinerId", value, d.GetAllocator ());
//
//                   value = block_it->GetBlockSizeBytes ();
//                   totalBlockMessageSize += value.GetInt();
//                   blockInfo.AddMember("size", value, d.GetAllocator ());
//
//                   value = block_it->GetTimeCreated ();
//                   blockInfo.AddMember("timeCreated", value, d.GetAllocator ());
//
//                   value = block_it->GetTimeReceived ();
//                   blockInfo.AddMember("timeReceived", value, d.GetAllocator ());
//
//                   array.PushBack(blockInfo, d.GetAllocator());
//                 }
//
//                 d.AddMember("blocks", array, d.GetAllocator());
//
//                 double sendTime = totalBlockMessageSize / m_uploadSpeed;
// 	            double eventTime;
//
// /*                 std::cout << "Node " << GetNode()->GetId() << "-" << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
// 		  		          << " " << m_peersDownloadSpeeds[Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()] << " Mbps , time = "
// 		  		          << Simulator::Now ().GetSeconds() << "s \n"; */
//
//                 if (m_sendBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendBlockTimes.back())
//                 {
//                   eventTime = 0;
//                 }
//                 else
//                 {
//                   //std::cout << "m_sendBlockTimes.back() = m_sendBlockTimes.back() = " << m_sendBlockTimes.back() << std::endl;
//                   eventTime = m_sendBlockTimes.back() - Simulator::Now ().GetSeconds();
//                 }
//                 m_sendBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);
//
//                 //std::cout << sendTime << " " << eventTime << " " << m_sendBlockTimes.size() << std::endl;
//                 NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
//                             << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");
//
//
//                 // Stringify the DOM
//                 rapidjson::StringBuffer packetInfo;
//                 rapidjson::Writer<rapidjson::StringBuffer> writer(packetInfo);
//                 d.Accept(writer);
//                 std::string packet = packetInfo.GetString();
//                 NS_LOG_INFO ("DEBUG: " << packetInfo.GetString());
//
//                 Simulator::Schedule (Seconds(eventTime), &BlockchainNode::SendBlock, this, packet, from);
//                 Simulator::Schedule (Seconds(eventTime + sendTime), &BlockchainNode::RemoveSendTime, this);
//
//               }
//               break;
//             }
//             case EXT_GET_DATA:
//             {
//               NS_LOG_INFO ("EXT_GET_DATA");
//
//               int j;
//               int totalChunkMessageSize = 0;
//               std::map<std::string, int>            requestedChunks;
//               std::vector<std::string>::iterator    chunk_it;
//
//               m_nodeStats->extGetDataReceivedBytes += m_bitcoinMessageHeader + m_countBytes + d["chunks"].Size()*m_inventorySizeBytes;
//
//               for (j=0; j<d["chunks"].Size(); j++)
//               {
//                 std::string            invDelimiter = "/";
//                 std::string            chunkHash = d["chunks"][j]["chunk"].GetString();
//                 std::string            chunkHashHelp = chunkHash.substr(0);
//                 std::ostringstream     help;
//                 std::string            blockHash;
//                 size_t                 invPos = chunkHashHelp.find(invDelimiter);
//                 std::vector<int>       candidateChunks;
//                 int                    blockSize = -1;
//
//                 int height = atoi(chunkHashHelp.substr(0, invPos).c_str());
//
//                 chunkHashHelp.erase(0, invPos + invDelimiter.length());
//                 invPos = chunkHashHelp.find(invDelimiter);
//                 int minerId = atoi(chunkHashHelp.substr(0, invPos).c_str());
//
//                 chunkHashHelp.erase(0, invPos + invDelimiter.length());
//                 int chunkId = atoi(chunkHashHelp.substr(0).c_str());
//                 help << height << "/" << minerId;
//                 blockHash = help.str();
//
//                 m_nodeStats->extGetDataReceivedBytes += 6; //1Byte(fullBlock) + 4Bytes(numberOfChunks) + 1Byte(requested chunk)
//                 if (!d["chunks"][j]["fullBlock"].GetBool())
//                   m_nodeStats->extGetDataReceivedBytes += d["chunks"][j]["availableChunks"].Size();
//
//                 if (m_blockchain.HasBlock(height, minerId) || m_blockchain.IsOrphan(height, minerId) || ReceivedButNotValidated(blockHash))
//                 {
//                   NS_LOG_INFO("EXT_GET_DATA: Bitcoin node " << GetNode ()->GetId ()
//                   << " has already received the block with height = "
//                   << height << " and minerId = " << minerId);
//                   requestedChunks[chunkHash] = -1;
//                 }
//                 else if (OnlyHeadersReceived(blockHash))
//                 {
//                   NS_LOG_INFO("EXT_GET_DATA: Bitcoin node " << GetNode ()->GetId ()
//                               << " has received the headers (and maybe some chunks) of the block with hash = " << blockHash);
//                   if (HasChunk(blockHash, chunkId))
//                     requestedChunks[chunkHash] = -1;
//                   blockSize = m_onlyHeadersReceived[blockHash].GetBlockSizeBytes();
//
//                   if (d["chunks"][j]["fullBlock"].GetBool())
//                   {
//                     for (auto &chunk : m_queueChunks[blockHash])
//                       candidateChunks.push_back(chunk);
//                   }
//                   else
//                   {
//                     for (int k = 0; k < d["chunks"][j]["availableChunks"].Size(); k++)
//                     {
//                       if (std::find(m_queueChunks[blockHash].begin(), m_queueChunks[blockHash].end(), d["chunks"][j]["availableChunks"][k].GetInt()) != m_queueChunks[blockHash].end())
//                         candidateChunks.push_back(d["chunks"][j]["availableChunks"][k].GetInt());
//                     }
//                   }
//                 }
//                 else
//                 {
//                   NS_LOG_INFO("EXT_GET_DATA: Bitcoin node " << GetNode ()->GetId ()
//                   << " does not have the block with height = "
//                   << height << " and minerId = " << minerId);
//                 }
//
//
// /*                     std::cout << "candidateChunks = ";
//                     for (auto chunk : candidateChunks)
//                       std::cout << chunk << ", ";
//                     std::cout << "\n"; */
//
//                 if (candidateChunks.size() > 0)
//                 {
//                   EventId              timeout;
//                   int randomIndex = rand() % candidateChunks.size();
//
//                   NS_LOG_INFO("EXT_GET_DATA: Bitcoin node " << GetNode ()->GetId ()
//                                << " will request the chunk with index = " << randomIndex << " and value = " << candidateChunks[randomIndex]);
//                   m_queueChunks[blockHash].erase(std::remove(m_queueChunks[blockHash].begin(),
//                                                              m_queueChunks[blockHash].end(), candidateChunks[randomIndex]),
//                                                              m_queueChunks[blockHash].end());
//
//                   std::ostringstream chunk;
//                   chunk << blockHash << "/" << candidateChunks[randomIndex];
//                   requestedChunks[chunkHash] = candidateChunks[randomIndex];
//
//
//                   if (blockSize == -1)
//                     NS_FATAL_ERROR ("blockSize == -1");
//
//                   timeout = Simulator::Schedule (Minutes(m_invTimeoutMinutes.GetMinutes() / ceil(blockSize/static_cast<double>(m_chunkSize))),
//                                                      &BlockchainNode::ChunkTimeoutExpired, this, chunk.str());
//
//                   m_chunkTimeouts[chunk.str()] = timeout;
//                   m_queueChunkPeers[blockHash].push_back(from);
//                 }
//                 else
//                 {
//                   NS_LOG_INFO("EXT_GET_DATA: Bitcoin node " << GetNode ()->GetId ()
//                               << " will not request any chunks from this peer, because it has already all the available ones");
//                 }
//               }
//
//
//               if (!requestedChunks.empty())
//               {
//                 rapidjson::Value value;
//                 rapidjson::Value chunkArray(rapidjson::kArrayType);
//
//                 d.RemoveMember("chunks");
//
//                 for (auto &requestedChunk : requestedChunks)
//                 {
//                   NS_LOG_INFO ("In requestedChunks " << requestedChunk.first);
//
//                   rapidjson::Value availableChunks(rapidjson::kArrayType);
//                   rapidjson::Value requestChunks(rapidjson::kArrayType);
//                   rapidjson::Value chunkInfo(rapidjson::kObjectType);
//
//                   std::string            invDelimiter = "/";
//                   std::ostringstream     help;
//                   std::string            blockHash;
//                   std::string            chunkHash = requestedChunk.first.substr(0);
//                   size_t                 invPos = chunkHash.find(invDelimiter);
//                   Block                  newBlock;
//                   int                    blockSize;
//                   int height = atoi(chunkHash.substr(0, invPos).c_str());
//
//                   chunkHash.erase(0, invPos + invDelimiter.length());
//                   invPos = chunkHash.find(invDelimiter);
//                   int minerId = atoi(chunkHash.substr(0, invPos).c_str());
//
//                   chunkHash.erase(0, invPos + invDelimiter.length());
//                   int chunkId = atoi(chunkHash.substr(0).c_str());
//                   help << height << "/" << minerId;
//                   blockHash = help.str();
//
//
//                   if (m_blockchain.HasBlock(height, minerId) || m_blockchain.IsOrphan(height, minerId))
//                   {
//                     newBlock = m_blockchain.ReturnBlock (height, minerId);
//                     value = true;
//                     chunkInfo.AddMember("fullBlock", value, d.GetAllocator ());
//                     blockSize = newBlock.GetBlockSizeBytes ();
//                   }
//                   else if (ReceivedButNotValidated(blockHash))
//                   {
//                     newBlock = m_receivedNotValidated[blockHash];
//                     value = true;
//                     chunkInfo.AddMember("fullBlock", value, d.GetAllocator ());
//                     blockSize = newBlock.GetBlockSizeBytes ();
//                   }
//                   else if (OnlyHeadersReceived(blockHash))
//                   {
//                     newBlock = m_onlyHeadersReceived[blockHash];
//                     blockSize = newBlock.GetBlockSizeBytes ();
//                     int noChunks = ceil(blockSize/static_cast<double>(m_chunkSize));
//
//                     if (m_receivedChunks[blockHash].size() == noChunks)
//                     {
//                       value = true;
//                       chunkInfo.AddMember("fullBlock", value, d.GetAllocator ());
//                       NS_LOG_DEBUG("1 " << m_receivedChunks[blockHash].size());
//                     }
//                     else
//                     {
//                       NS_LOG_DEBUG("2 " << m_receivedChunks[blockHash].size());
//
//                       value = false;
//                       chunkInfo.AddMember("fullBlock", value, d.GetAllocator ());
//
//                       for (auto &c : m_receivedChunks[blockHash])
//                       {
//                         value = c;
//                         availableChunks.PushBack(value, d.GetAllocator());
//                       }
//                       chunkInfo.AddMember("availableChunks", availableChunks, d.GetAllocator ());
//                     }
//                   }
//
//                   value = newBlock.GetBlockHeight ();
//                   chunkInfo.AddMember("height", value, d.GetAllocator ());
//
//                   value = newBlock.GetMinerId ();
//                   chunkInfo.AddMember("minerId", value, d.GetAllocator ());
//
//                   value = chunkId;
//                   chunkInfo.AddMember("chunk", value, d.GetAllocator ());
//
//                   value = newBlock.GetParentBlockMinerId ();
//                   chunkInfo.AddMember("parentBlockMinerId", value, d.GetAllocator ());
//
//                   value = newBlock.GetBlockSizeBytes ();
//                   if (chunkId == ceil(newBlock.GetBlockSizeBytes () / static_cast<double>(m_chunkSize) - 1) &&
//                       newBlock.GetBlockSizeBytes () % m_chunkSize > 0)
//                     totalChunkMessageSize += newBlock.GetBlockSizeBytes () % m_chunkSize;
//                   else
//                     totalChunkMessageSize += m_chunkSize;
//
//                   chunkInfo.AddMember("size", value, d.GetAllocator ());
//
//                   value = newBlock.GetTimeCreated ();
//                   chunkInfo.AddMember("timeCreated", value, d.GetAllocator ());
//
//                   value = newBlock.GetTimeReceived ();
//                   chunkInfo.AddMember("timeReceived", value, d.GetAllocator ());
//
//                   if (requestedChunk.second != -1)
//                   {
//                     value = requestedChunk.second;
//                     requestChunks.PushBack(value, d.GetAllocator());
//                   }
//                   chunkInfo.AddMember("requestChunks", requestChunks, d.GetAllocator ());
//
// /*                  //Test chunk to chunk messages
//                   value = 1;
//                   requestChunks.PushBack(value, d.GetAllocator());
//                   chunkInfo.AddMember("requestChunks", requestChunks, d.GetAllocator ()); */
//
//                   chunkArray.PushBack(chunkInfo, d.GetAllocator());
//                 }
//
//                 d.AddMember("chunks", chunkArray, d.GetAllocator());
//
//                 double sendTime = totalChunkMessageSize / m_uploadSpeed;
//                 double eventTime;
//
// /*                 std::cout << "Node " << GetNode()->GetId() << "-" << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
// 		  		          << " " << m_peersDownloadSpeeds[Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()] << " Mbps , time = "
// 		  		          << Simulator::Now ().GetSeconds() << "s \n"; */
//
//                 if (m_sendBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendBlockTimes.back())
//                 {
//                   eventTime = 0;
//                 }
//                 else
//                 {
//                   //std::cout << "m_sendBlockTimes.back() = m_sendBlockTimes.back() = " << m_sendBlockTimes.back() << std::endl;
//                   eventTime = m_sendBlockTimes.back() - Simulator::Now ().GetSeconds();
//                 }
//                 m_sendBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);
//
//                 //std::cout << sendTime << " " << eventTime << " " << m_sendBlockTimes.size() << std::endl;
//                 NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the chunk to " << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
//                             << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");
//
//
//                 // Stringify the DOM
//                 rapidjson::StringBuffer packetInfo;
//                 rapidjson::Writer<rapidjson::StringBuffer> writer(packetInfo);
//                 d.Accept(writer);
//                 std::string packet = packetInfo.GetString();
//                 NS_LOG_INFO ("DEBUG: " << packetInfo.GetString());
//
//                 Simulator::Schedule (Seconds(eventTime), &BlockchainNode::SendChunk, this, packet, from);
//                 Simulator::Schedule (Seconds(eventTime + sendTime), &BlockchainNode::RemoveSendTime, this);
//               }
//               break;
//             }
//             case HEADERS:
//             {
//               NS_LOG_INFO ("HEADERS");
//
//               std::vector<std::string>              requestHeaders;
//               std::vector<std::string>              requestBlocks;
//               std::vector<std::string>::iterator    block_it;
//               int j;
//
//               m_nodeStats->headersReceivedBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
//
//
//               for (j=0; j<d["blocks"].Size(); j++)
//               {
//                 int parentHeight = d["blocks"][j]["height"].GetInt() - 1;
//                 int parentMinerId = d["blocks"][j]["parentBlockMinerId"].GetInt();
//                 int height = d["blocks"][j]["height"].GetInt();
//                 int minerId = d["blocks"][j]["minerId"].GetInt();
//
//
//                 EventId              timeout;
//                 std::ostringstream   stringStream;
//                 std::string          blockHash;
//                 std::string          parentBlockHash ;
//
//                 stringStream << height << "/" << minerId;
//                 blockHash = stringStream.str();
//                 Block newBlockHeaders(d["blocks"][j]["height"].GetInt(), d["blocks"][j]["minerId"].GetInt(), d["blocks"][j]["parentBlockMinerId"].GetInt(),
//                                       d["blocks"][j]["size"].GetInt(), d["blocks"][j]["timeCreated"].GetDouble(),
//                                       d["blocks"][j]["nodeId"].GetInt(),
//                                       d["blocks"][j]["nodePublicKey"].GetString(),
//                                       d["blocks"][j]["signature"].GetString(),
//                                       Simulator::Now ().GetSeconds (), Inet6SocketAddress::ConvertFrom(from).GetIpv6 ());
//                 m_onlyHeadersReceived[blockHash] = Block (d["blocks"][j]["height"].GetInt(), d["blocks"][j]["minerId"].GetInt(), d["blocks"][j]["parentBlockMinerId"].GetInt(),
//                                                           d["blocks"][j]["size"].GetInt(), d["blocks"][j]["timeCreated"].GetDouble(),               d["blocks"][j]["nodeId"].GetInt(),d["blocks"][j]["nodePublicKey"].GetString(),d["blocks"][j]["signature"].GetString(),Simulator::Now ().GetSeconds (), Inet6SocketAddress::ConvertFrom(from).GetIpv6 ());
//                 //PrintOnlyHeadersReceived();
//
//                 stringStream.clear();
//                 stringStream.str("");
//
//                 stringStream << parentHeight << "/" << parentMinerId;
//                 parentBlockHash = stringStream.str();
//
//                 if(m_protocolType == SENDHEADERS && !m_blockchain.HasBlock(height, minerId) && !m_blockchain.IsOrphan(height, minerId) && !ReceivedButNotValidated(blockHash))
//                 {
//                   NS_LOG_INFO("We have not received an INV for the block with height = " << d["blocks"][j]["height"].GetInt()
//                                << " and minerId = " << d["blocks"][j]["minerId"].GetInt());
//
//                   /**
//                    * Acquire block
//                    */
//
//                   if (m_invTimeouts.find(blockHash) == m_invTimeouts.end())
//                   {
//                     NS_LOG_INFO("HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has not requested the block yet");
//                     requestBlocks.push_back(blockHash.c_str());
//                     timeout = Simulator::Schedule (m_invTimeoutMinutes, &BlockchainNode::InvTimeoutExpired, this, blockHash);
//                     m_invTimeouts[blockHash] = timeout;
//                   }
//                   else
//                   {
//                     NS_LOG_INFO("HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has already requested the block");
//                   }
//
//                   m_queueInv[blockHash].push_back(from);
//
//                 }
//
//
//                 if (!m_blockchain.HasBlock(parentHeight, parentMinerId) && !m_blockchain.IsOrphan(parentHeight, parentMinerId) && !ReceivedButNotValidated(parentBlockHash))
//                 {
//                   NS_LOG_INFO("The Block with height = " << d["blocks"][j]["height"].GetInt()
//                                << " and minerId = " << d["blocks"][j]["minerId"].GetInt()
//                                << " is an orphan\n");
//
//                   /**
//                    * Acquire parent
//                    */
//
//                   if (m_invTimeouts.find(parentBlockHash) == m_invTimeouts.end())
//                   {
//                     NS_LOG_INFO("HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has not requested its parent block yet");
//
//                     if(m_protocolType == STANDARD_PROTOCOL ||
//                       (m_protocolType == SENDHEADERS && std::find(requestBlocks.begin(), requestBlocks.end(), parentBlockHash) == requestBlocks.end()))
//                     {
//                       if (!OnlyHeadersReceived(parentBlockHash))
//                         requestHeaders.push_back(parentBlockHash.c_str());
//                       timeout = Simulator::Schedule (m_invTimeoutMinutes, &BlockchainNode::InvTimeoutExpired, this, parentBlockHash);
//                       m_invTimeouts[parentBlockHash] = timeout;
//                     }
//                   }
//                   else
//                   {
//                     NS_LOG_INFO("HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has already requested the block");
//                   }
//
//                   if(m_protocolType == STANDARD_PROTOCOL ||
//                     (m_protocolType == SENDHEADERS && std::find(requestBlocks.begin(), requestBlocks.end(), parentBlockHash) == requestBlocks.end()))
//                     m_queueInv[parentBlockHash].push_back(from);
//
//                   //PrintQueueInv();
//                   //PrintInvTimeouts();
//
//                 }
//                 else
//                 {
//                   /**
// 	               * Block is not orphan, so we can go on validating
// 	               */
//                   NS_LOG_INFO("The Block with height = " << d["blocks"][j]["height"].GetInt()
//                               << " and minerId = " << d["blocks"][j]["minerId"].GetInt()
//                               << " is NOT an orphan\n");
//                 }
//               }
//
//               if (!requestHeaders.empty())
//               {
//                 rapidjson::Value   value;
//                 rapidjson::Value   array(rapidjson::kArrayType);
//                 Time               timeout;
//
//                 d.RemoveMember("blocks");
//
//                 for (block_it = requestHeaders.begin(); block_it < requestHeaders.end(); block_it++)
//                 {
//                   value.SetString(block_it->c_str(), block_it->size(), d.GetAllocator());
//                   array.PushBack(value, d.GetAllocator());
//                 }
//
//                 d.AddMember("blocks", array, d.GetAllocator());
//
//
//                 SendMessage(HEADERS, GET_HEADERS, d, from);
//                 SendMessage(HEADERS, GET_DATA, d, from);
//               }
//
//               if (!requestBlocks.empty())
//               {
//                 rapidjson::Value   value;
//                 rapidjson::Value   array(rapidjson::kArrayType);
//                 Time               timeout;
//
//                 d.RemoveMember("blocks");
//
//                 for (block_it = requestBlocks.begin(); block_it < requestBlocks.end(); block_it++)
//                 {
//                   value.SetString(block_it->c_str(), block_it->size(), d.GetAllocator());
//                   array.PushBack(value, d.GetAllocator());
//                 }
//
//                 d.AddMember("blocks", array, d.GetAllocator());
//
//                 SendMessage(HEADERS, GET_DATA, d, from);
//               }
//               break;
//             }
//             case EXT_HEADERS:
//             {
//               NS_LOG_INFO ("EXT_HEADERS");
//
//               std::vector<std::string>              requestHeaders;
//               std::vector<std::string>              requestChunks;
//               std::vector<std::string>::iterator    block_it;
//               int j;
//
//               m_nodeStats->extHeadersReceivedBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
//
//
//               for (j=0; j<d["blocks"].Size(); j++)
//               {
//                 int parentHeight = d["blocks"][j]["height"].GetInt() - 1;
//                 int parentMinerId = d["blocks"][j]["parentBlockMinerId"].GetInt();
//                 int height = d["blocks"][j]["height"].GetInt();
//                 int minerId = d["blocks"][j]["minerId"].GetInt();
//                 int blockSize = d["blocks"][j]["size"].GetInt();
//
//
//                 EventId              timeout;
//                 std::ostringstream   stringStream;
//                 std::string          blockHash;
//                 std::string          parentBlockHash ;
//
//                 m_nodeStats->extHeadersReceivedBytes += 1;//fullBlock
//                 if (!d["blocks"][j]["fullBlock"].GetBool())
//                   m_nodeStats->extHeadersReceivedBytes += d["blocks"][j]["availableChunks"].Size();
//
//                 stringStream << height << "/" << minerId;
//                 blockHash = stringStream.str();
//                 Block newBlockHeaders(d["blocks"][j]["height"].GetInt(), d["blocks"][j]["minerId"].GetInt(), d["blocks"][j]["parentBlockMinerId"].GetInt(),
//                                                          d["blocks"][j]["size"].GetInt(), d["blocks"][j]["timeCreated"].GetDouble(), d["blocks"][j]["nodeId"].GetInt(),d["blocks"][j]["nodePublicKey"].GetString(), d["blocks"][j]["signature"].GetString(), Simulator::Now ().GetSeconds (), Inet6SocketAddress::ConvertFrom(from).GetIpv6 ());
//                 if (!OnlyHeadersReceived(blockHash))
//                 {
//                   m_onlyHeadersReceived[blockHash] = Block (d["blocks"][j]["height"].GetInt(), d["blocks"][j]["minerId"].GetInt(), d["blocks"][j]["parentBlockMinerId"].GetInt(),
//                                                             d["blocks"][j]["size"].GetInt(), d["blocks"][j]["timeCreated"].GetDouble(), d["blocks"][j]["nodeId"].GetInt(), d["blocks"][j]["nodePublicKey"].GetString(), d["blocks"][j]["signature"].GetString(),
//                                                             Simulator::Now ().GetSeconds (), Inet6SocketAddress::ConvertFrom(from).GetIpv6 ());
//                 }
//                 //PrintOnlyHeadersReceived();
//
//                 stringStream.clear();
//                 stringStream.str("");
//
//                 stringStream << parentHeight << "/" << parentMinerId;
//                 parentBlockHash = stringStream.str();
//
//                 if(!m_blockchain.HasBlock(height, minerId) && !m_blockchain.IsOrphan(height, minerId) && !ReceivedButNotValidated(blockHash))
//                 {
// /*                   NS_LOG_INFO("We have not received an INV for the block with height = " << d["blocks"][j]["height"].GetInt()
//                                << " and minerId = " << d["blocks"][j]["minerId"].GetInt()); */
//
//                   NS_LOG_INFO("EXT_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                               << " does not have the block with height = "
//                               << height << " and minerId = " << minerId);
//
//                   if (m_queueChunks.find(blockHash) == m_queueChunks.end())
//                   {
//                     NS_LOG_INFO("EXT_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                 << " does not have an entry in m_queueChunks");
//                     for (int i = 0; i < ceil(blockSize/static_cast<double>(m_chunkSize)); i++)
//                       m_queueChunks[blockHash].push_back(i);
//                   }
//                   //PrintQueueChunks();
//
//
//                   /**
//                    * Check if we have already requested all the chunks
//                    */
//
//                   if (m_queueChunks[blockHash].size() > 0)
//                   {
//                     NS_LOG_INFO("EXT_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has not requested all the chunks yet");
//
//
//                     std::vector<int> candidateChunks;
//                     if (d["blocks"][j]["fullBlock"].GetBool())
//                     {
//                       for (auto &chunk : m_queueChunks[blockHash])
//                         candidateChunks.push_back(chunk);
//                     }
//                     else
//                     {
//                       for (int k = 0; k < d["blocks"][j]["availableChunks"].Size(); k++)
//                       {
//                         if (std::find(m_queueChunks[blockHash].begin(), m_queueChunks[blockHash].end(), d["blocks"][j]["availableChunks"][k].GetInt()) != m_queueChunks[blockHash].end())
//                           candidateChunks.push_back(d["blocks"][j]["availableChunks"][k].GetInt());
//                       }
//                     }
//
// /*                     std::cout << "candidateChunks = ";
//                     for (auto chunk : candidateChunks)
//                       std::cout << chunk << ", ";
//                     std::cout << "\n"; */
//
//                     if (candidateChunks.size() > 0 &&
//                         std::find(m_queueChunkPeers[blockHash].begin(), m_queueChunkPeers[blockHash].end(), from) == m_queueChunkPeers[blockHash].end())
//                     {
//                       int randomIndex = rand() % candidateChunks.size();
//                       NS_LOG_INFO("EXT_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                   << " will request the chunk with index = " << randomIndex << " and value = " << candidateChunks[randomIndex]);
//                       m_queueChunks[blockHash].erase(std::remove(m_queueChunks[blockHash].begin(),
//                                                                  m_queueChunks[blockHash].end(), candidateChunks[randomIndex]),
//                                                                  m_queueChunks[blockHash].end());
//
//                       std::ostringstream chunk;
//                       chunk << blockHash << "/" << candidateChunks[randomIndex];
//                       requestChunks.push_back(chunk.str());
//
//                       timeout = Simulator::Schedule (Minutes(m_invTimeoutMinutes.GetMinutes() / ceil(blockSize/static_cast<double>(m_chunkSize))),
//                                                      &BlockchainNode::ChunkTimeoutExpired, this, chunk.str());
//
//                       m_chunkTimeouts[chunk.str()] = timeout;
//                       m_queueChunkPeers[blockHash].push_back(from);
//                     }
//                     else
//                     {
//                       if (std::find(m_queueChunkPeers[blockHash].begin(), m_queueChunkPeers[blockHash].end(), from) == m_queueChunkPeers[blockHash].end())
//                         NS_LOG_INFO("EXT_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                     << " will not request any chunks from this peer, because it has already all the available ones");
//                       else
//                         NS_LOG_INFO("EXT_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                      << " has already requested a chunk from this peer");
//
//                     }
//
// /*                     PrintQueueChunks();
//                     PrintChunkTimeouts();
//                     PrintQueueChunkPeers();
//                     PrintReceivedChunks(); */
//                   }
//                   else
//                   {
//                     NS_LOG_INFO("EXT_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has already requested a chunk from this peer");
//                   }
//
//                 }
//                 else
//                 {
//                   /**
//                    * Block is not orphan, so we can go on validating
//                    */
//                   NS_LOG_INFO("The Block with height = " << d["blocks"][j]["height"].GetInt()
//                               << " and minerId = " << d["blocks"][j]["minerId"].GetInt()
//                               << " has already been received\n");
//                 }
//
//                 if (!m_blockchain.HasBlock(parentHeight, parentMinerId) && !m_blockchain.IsOrphan(parentHeight, parentMinerId) && !ReceivedButNotValidated(parentBlockHash))
//                 {
//                   NS_LOG_INFO("The Block with height = " << d["blocks"][j]["height"].GetInt()
//                                << " and minerId = " << d["blocks"][j]["minerId"].GetInt()
//                                << " is an orphan\n");
//
//                   /**
//                    * Acquire parent
//                    */
//
//                   if (m_queueChunks.find(parentBlockHash) == m_queueChunks.end() ||
//                       std::find(m_queueChunkPeers[parentBlockHash].begin(), m_queueChunkPeers[parentBlockHash].end(), from) == m_queueChunkPeers[parentBlockHash].end())
//                   {
//                     NS_LOG_INFO("EXT_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has not requested parent block chunks from this peer yet");
//                       requestHeaders.push_back(parentBlockHash.c_str());
//                   }
//                   else
//                   {
//                     NS_LOG_INFO("EXT_HEADERS: Bitcoin node " << GetNode ()->GetId ()
//                                  << " has already requested the block");
//                   }
//
//                   if(m_protocolType == STANDARD_PROTOCOL ||
//                     (m_protocolType == SENDHEADERS && std::find(requestChunks.begin(), requestChunks.end(), parentBlockHash) == requestChunks.end()))
//                     m_queueInv[parentBlockHash].push_back(from);
//
//                   //PrintQueueInv();
//                   //PrintInvTimeouts();
//
//                 }
//                 else
//                 {
//                   /**
// 	               * Block is not orphan, so we can go on validating
// 	               */
//                   NS_LOG_INFO("The Block with height = " << d["blocks"][j]["height"].GetInt()
//                               << " and minerId = " << d["blocks"][j]["minerId"].GetInt()
//                               << " is NOT an orphan\n");
//                 }
//               }
//
//               if (!requestHeaders.empty())
//               {
//                 rapidjson::Value   value;
//                 rapidjson::Value   array(rapidjson::kArrayType);
//                 Time               timeout;
//
//                 d.RemoveMember("blocks");
//
//                 for (block_it = requestHeaders.begin(); block_it < requestHeaders.end(); block_it++)
//                 {
//                   value.SetString(block_it->c_str(), block_it->size(), d.GetAllocator());
//                   array.PushBack(value, d.GetAllocator());
//                 }
//
//                 d.AddMember("blocks", array, d.GetAllocator());
//
//
//                 SendMessage(EXT_HEADERS, EXT_GET_HEADERS, d, from);
//               }
//
//               if (!requestChunks.empty())
//               {
//                 rapidjson::Value   value;
//                 rapidjson::Value   chunkArray(rapidjson::kArrayType);
//                 rapidjson::Value   availableChunks(rapidjson::kArrayType);
//                 rapidjson::Value   chunkInfo(rapidjson::kObjectType);
//
//                 d.RemoveMember("type");
//                 d.RemoveMember("blocks");
//
//                 value.SetString("chunk");
//                 d.AddMember("type", value, d.GetAllocator());
//
//                 for (auto chunk_it = requestChunks.begin(); chunk_it < requestChunks.end(); chunk_it++)
//                 {
//
//                   std::string            invDelimiter = "/";
//                   std::string            chunkHash = *chunk_it;
//                   std::string            chunkHashHelp = chunkHash.substr(0);
//                   std::ostringstream     help;
//                   std::string            blockHash;
//                   size_t                 invPos = chunkHashHelp.find(invDelimiter);
//
//                   int height = atoi(chunkHashHelp.substr(0, invPos).c_str());
//
//                   chunkHashHelp.erase(0, invPos + invDelimiter.length());
//                   invPos = chunkHashHelp.find(invDelimiter);
//                   int minerId = atoi(chunkHashHelp.substr(0, invPos).c_str());
//
//                   chunkHashHelp.erase(0, invPos + invDelimiter.length());
//                   int chunkId = atoi(chunkHashHelp.substr(0).c_str());
//                   help << height << "/" << minerId;
//                   blockHash = help.str();
//
//                   if (m_receivedChunks.find(blockHash) != m_receivedChunks.end())
//                   {
//                     for ( auto k : m_receivedChunks[blockHash])
//                     {
//                       value = k;
//                       availableChunks.PushBack(value, d.GetAllocator());
//                     }
//                   }
//                   chunkInfo.AddMember("availableChunks", availableChunks, d.GetAllocator());
//
//                   value = false;
//                   chunkInfo.AddMember("fullBlock", value, d.GetAllocator());
//
//                   value.SetString(chunk_it->c_str(), chunk_it->size(), d.GetAllocator());
//                   chunkInfo.AddMember("chunk", value, d.GetAllocator());
//
//                   chunkArray.PushBack(chunkInfo, d.GetAllocator());
//                 }
//                 d.AddMember("chunks", chunkArray, d.GetAllocator());
//
//                 SendMessage(EXT_HEADERS, EXT_GET_DATA, d, from);
//
//               }
//               break;
//             }
            case BLOCK:
            {
              NS_LOG_INFO ("BLOCK");
              int blockMessageSize = 0;
              double receiveTime = 0;
              double eventTime = 0;
              double minSpeed = std::min(m_downloadSpeed, m_peersUploadSpeeds[Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()] * 1000000 / 8);

              std::string blockType = d["type"].GetString();

              blockMessageSize += m_bitcoinMessageHeader;

              for (int j=0; j<d["blocks"].Size(); j++)
              {
                if (blockType == "block")
                  blockMessageSize += d["blocks"][j]["size"].GetInt();
                else if (blockType == "compressed-block")
                {
                  int    noTransactions = static_cast<int>((d["blocks"][j]["size"].GetInt() - m_blockHeadersSizeBytes)/m_averageTransactionSize);
                  long   blockSize = m_blockHeadersSizeBytes + m_transactionIndexSize*noTransactions;
                  blockMessageSize += blockSize;
                }
              }

              m_nodeStats->blockReceivedBytes += blockMessageSize;

              // Stringify the DOM
              rapidjson::StringBuffer blockInfo;
              rapidjson::Writer<rapidjson::StringBuffer> blockWriter(blockInfo);
              d.Accept(blockWriter);


              NS_LOG_INFO("BLOCK: At time " << Simulator::Now ().GetSeconds ()
                          << " Node " << GetNode()->GetId() << " received a block message " << blockInfo.GetString());
              NS_LOG_INFO(m_downloadSpeed << " " << m_peersUploadSpeeds[Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()] * 1000000 / 8 << " " << minSpeed);

              std::string help = blockInfo.GetString();

              if (blockType == "block")
              {
                if (m_receiveBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_receiveBlockTimes.back())
                {
                  receiveTime = blockMessageSize / m_downloadSpeed;
                  eventTime = blockMessageSize / minSpeed;
                }
                else
                {
                  receiveTime = blockMessageSize / m_downloadSpeed + m_receiveBlockTimes.back() - Simulator::Now ().GetSeconds();
                  eventTime = blockMessageSize / minSpeed + m_receiveBlockTimes.back() - Simulator::Now ().GetSeconds();
                }
                m_receiveBlockTimes.push_back(Simulator::Now ().GetSeconds() + receiveTime);


                Simulator::Schedule (Seconds(eventTime), &BlockchainNode::ReceivedBlockMessage, this, help, from);
                Simulator::Schedule (Seconds(receiveTime), &BlockchainNode::RemoveReceiveTime, this);
              }
              else if (blockType == "compressed-block")
              {
                if (m_receiveCompressedBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_receiveCompressedBlockTimes.back())
                {
                  receiveTime = blockMessageSize / m_downloadSpeed;
                  eventTime = blockMessageSize / minSpeed;
                }
                else
                {
                  receiveTime = blockMessageSize / m_downloadSpeed + m_receiveCompressedBlockTimes.back() - Simulator::Now ().GetSeconds();
                  eventTime = blockMessageSize / minSpeed + m_receiveCompressedBlockTimes.back() - Simulator::Now ().GetSeconds();
                }
                m_receiveCompressedBlockTimes.push_back(Simulator::Now ().GetSeconds() + receiveTime);


                Simulator::Schedule (Seconds(eventTime), &BlockchainNode::ReceivedBlockMessage, this, help, from);
                Simulator::Schedule (Seconds(receiveTime), &BlockchainNode::RemoveCompressedBlockReceiveTime, this);
              }

              NS_LOG_INFO("BLOCK:  Node " << GetNode()->GetId() << " will receive the full block message at " << Simulator::Now ().GetSeconds() + eventTime);

              break;
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
                     << "s bitcoin node " << GetNode ()->GetId () << " received "
                     <<  packet->GetSize () << " bytes from "
                     << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                     << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ());
      }
      m_rxTrace (packet, from);
  }
}


void
BlockchainNode::ReceivedBlockMessage(std::string &blockInfo, Address &from)
{
  NS_LOG_FUNCTION (this);

  rapidjson::Document d;
  d.Parse(blockInfo.c_str());

  NS_LOG_INFO("ReceivedBlockMessage: At time " << Simulator::Now ().GetSeconds ()
              << " Node " << GetNode()->GetId() << " received a block message " << blockInfo);

  //m_receiveBlockTimes.erase(m_receiveBlockTimes.begin());

  for (int j=0; j<d["blocks"].Size(); j++)
  {
    int parentHeight = d["blocks"][j]["height"].GetInt() - 1;
    int parentMinerId = d["blocks"][j]["parentBlockMinerId"].GetInt();
    int height = d["blocks"][j]["height"].GetInt();
    int minerId = d["blocks"][j]["minerId"].GetInt();


    EventId              timeout;
    std::ostringstream   stringStream;
    std::string          blockHash;
    std::string          parentBlockHash;

    stringStream << height << "/" << minerId;
    blockHash = stringStream.str();

    if (m_onlyHeadersReceived.find(blockHash) != m_onlyHeadersReceived.end())
      m_onlyHeadersReceived.erase(blockHash);
    if (m_queueChunkPeers.find(blockHash) != m_queueChunkPeers.end())
      m_queueChunkPeers.erase (blockHash);
    if (m_queueChunks.find(blockHash) != m_queueChunks.end())
      m_queueChunks.erase (blockHash);
    if (m_receivedChunks.find(blockHash) != m_receivedChunks.end())
      m_receivedChunks.erase (blockHash);

    stringStream.clear();
    stringStream.str("");

    stringStream << parentHeight << "/" << parentMinerId;
    parentBlockHash = stringStream.str();

    if (!m_blockchain.HasBlock(parentHeight, parentMinerId) && !m_blockchain.IsOrphan(parentHeight, parentMinerId)
        && !ReceivedButNotValidated(parentBlockHash) && !OnlyHeadersReceived(parentBlockHash))
    {
      NS_LOG_INFO("The Block with height = " << d["blocks"][j]["height"].GetInt()
                 << " and minerId = " << d["blocks"][j]["minerId"].GetInt()
                 << " is an orphan, so it will be discarded\n");

      m_queueInv.erase(blockHash);
      Simulator::Cancel (m_invTimeouts[blockHash]);
      m_invTimeouts.erase(blockHash);
    }
    // else
    // {
    //   Block newBlock (d["blocks"][j]["height"].GetInt(), d["blocks"][j]["minerId"].GetInt(), d["blocks"][j]["parentBlockMinerId"].GetInt(),
    //                   d["blocks"][j]["size"].GetInt(), d["blocks"][j]["timeCreated"].GetDouble(),
    //                   d["blocks"][j]["nodeId"].GetInt(),
    //                   d["blocks"][j]["nodePublicKey"].GetString(),
    //                   d["blocks"][j]["signature"].GetString(),
    //                   Simulator::Now ().GetSeconds (), Inet6SocketAddress::ConvertFrom(from).GetIpv6 ());
    //
    //   ReceiveBlock (newBlock);
    // }
  }
}


void
BlockchainNode::ReceiveBlock(const Block &newBlock)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("ReceiveBlock: At time " << Simulator::Now ().GetSeconds ()
                << "s bitcoin node " << GetNode ()->GetId () << " received " << newBlock);

  std::ostringstream   stringStream;
  std::string          blockHash = stringStream.str();

  stringStream << newBlock.GetBlockHeight() << "/" << newBlock.GetMinerId();
  blockHash = stringStream.str();

  if (m_blockchain.HasBlock(newBlock) || m_blockchain.IsOrphan(newBlock) || ReceivedButNotValidated(blockHash))
  {
    NS_LOG_INFO ("ReceiveBlock: Bitcoin node " << GetNode ()->GetId () << " has already added this block in the m_blockchain: " << newBlock);

    if (m_invTimeouts.find(blockHash) != m_invTimeouts.end())
    {
      m_queueInv.erase(blockHash);
      Simulator::Cancel (m_invTimeouts[blockHash]);
      m_invTimeouts.erase(blockHash);
    }
  }
  else
  {
    NS_LOG_INFO ("ReceiveBlock: Bitcoin node " << GetNode ()->GetId () << " has NOT added this block in the m_blockchain: " << newBlock);

    m_receivedNotValidated[blockHash] = newBlock;
	//PrintQueueInv();
	//PrintInvTimeouts();

    if (m_invTimeouts.find(blockHash) != m_invTimeouts.end())
    {
      m_queueInv.erase(blockHash);
      Simulator::Cancel (m_invTimeouts[blockHash]);
      m_invTimeouts.erase(blockHash);
    }

    //PrintQueueInv();
	//PrintInvTimeouts();
    ValidateBlock (newBlock);
  }
}



void
BlockchainNode::SendBlock(std::string packetInfo, Address& from)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("SendBlock: At time " << Simulator::Now ().GetSeconds ()
                << "s bitcoin node " << GetNode ()->GetId () << " sent "
                << packetInfo << " to " << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ());

  //m_sendBlockTimes.erase(m_sendBlockTimes.begin());
  SendMessage(GET_DATA, BLOCK, packetInfo, from);
}



void
BlockchainNode::ReceivedHigherBlock(const Block &newBlock)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO("ReceivedHigherBlock: Bitcoin node " << GetNode ()->GetId () << " added a new block in the m_blockchain with higher height: " << newBlock);
}


void
BlockchainNode::ValidateBlock(const Block &newBlock)
{
  NS_LOG_FUNCTION (this);

  const Block *parent = m_blockchain.GetParent(newBlock);

  if (parent == nullptr)
  {
    NS_LOG_INFO("ValidateBlock: Block " << newBlock << " is an orphan\n");

	 m_blockchain.AddOrphan(newBlock);
	 //m_blockchain.PrintOrphans();
  }
  else
  {
    NS_LOG_INFO("ValidateBlock: Block's " << newBlock << " parent is " << *parent << "\n");

    /**
     * Block is not orphan, so we can go on validating
     */

    const int averageBlockSizeBytes = 458263;
    const double averageValidationTimeSeconds = 0.174;
    double validationTime = averageValidationTimeSeconds * newBlock.GetBlockSizeBytes() / averageBlockSizeBytes;

    Simulator::Schedule (Seconds(validationTime), &BlockchainNode::AfterBlockValidation, this, newBlock);
    NS_LOG_INFO ("ValidateBlock: The Block " << newBlock << " will be validated in "
                 << validationTime << "s");
  }

}

void
BlockchainNode::AfterBlockValidation(const Block &newBlock)
{
  NS_LOG_FUNCTION (this);

  int height = newBlock.GetBlockHeight();
  int minerId = newBlock.GetMinerId();
  std::ostringstream   stringStream;
  std::string          blockHash = stringStream.str();

  stringStream << height << "/" << minerId;
  blockHash = stringStream.str();

  RemoveReceivedButNotValidated(blockHash);

  NS_LOG_INFO ("AfterBlockValidation: At time " << Simulator::Now ().GetSeconds ()
               << "s bitcoin node " << GetNode ()->GetId ()
               << " validated block " <<  newBlock);

  if (newBlock.GetBlockHeight() > m_blockchain.GetBlockchainHeight())
    ReceivedHigherBlock(newBlock);

  if (m_blockchain.IsOrphan(newBlock))
  {
    NS_LOG_INFO ("AfterBlockValidation: Block " << newBlock << " was orphan");
	m_blockchain.RemoveOrphan(newBlock);
  }

  /**
   * Add Block in the blockchain.
   * Update m_meanBlockReceiveTime with the timeReceived of the newly received block.
   */

  m_meanBlockReceiveTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockReceiveTime
                         + (newBlock.GetTimeReceived() - m_previousBlockReceiveTime)/(m_blockchain.GetTotalBlocks());
  m_previousBlockReceiveTime = newBlock.GetTimeReceived();

  m_meanBlockPropagationTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockPropagationTime
                             + (newBlock.GetTimeReceived() - newBlock.GetTimeCreated())/(m_blockchain.GetTotalBlocks());

  m_meanBlockSize = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockSize
                  + (newBlock.GetBlockSizeBytes())/static_cast<double>(m_blockchain.GetTotalBlocks());

  m_blockchain.AddBlock(newBlock);

  if (!m_blockTorrent)
    AdvertiseNewBlock(newBlock);
  else
    AdvertiseFullBlock(newBlock);

  ValidateOrphanChildren(newBlock);

}


void
BlockchainNode::ValidateOrphanChildren(const Block &newBlock)
{
  NS_LOG_FUNCTION (this);

  std::vector<const Block *> children = m_blockchain.GetOrphanChildrenPointers(newBlock);

  if (children.size() == 0)
  {
    NS_LOG_INFO("ValidateOrphanChildren: Block " << newBlock << " has no orphan children\n");
  }
  else
  {
    std::vector<const Block *>::iterator  block_it;
	NS_LOG_INFO("ValidateOrphanChildren: Block " << newBlock << " has orphan children:");

	for (block_it = children.begin();  block_it < children.end(); block_it++)
    {
       NS_LOG_INFO ("\t" << **block_it);
       ValidateBlock (**block_it);
    }
  }
}


void
BlockchainNode::AdvertiseNewBlock (const Block &newBlock)
{
  NS_LOG_FUNCTION (this);

  rapidjson::Document d;
  rapidjson::Value value;
  rapidjson::Value array(rapidjson::kArrayType);
  std::ostringstream stringStream;
  std::string blockHash = stringStream.str();
  d.SetObject();

  value.SetString("block");
  d.AddMember("type", value, d.GetAllocator());

  if (m_protocolType == STANDARD_PROTOCOL)
  {
    value = INV;
    d.AddMember("message", value, d.GetAllocator());

    stringStream << newBlock.GetBlockHeight () << "/" << newBlock.GetMinerId ();
    blockHash = stringStream.str();
    value.SetString(blockHash.c_str(), blockHash.size(), d.GetAllocator());
    array.PushBack(value, d.GetAllocator());
    d.AddMember("inv", array, d.GetAllocator());
  }
  else if (m_protocolType == SENDHEADERS)
  {
    rapidjson::Value blockInfo(rapidjson::kObjectType);

    value = HEADERS;
    d.AddMember("message", value, d.GetAllocator());

    value = newBlock.GetBlockHeight ();
    blockInfo.AddMember("height", value, d.GetAllocator ());

    value = newBlock.GetMinerId ();
    blockInfo.AddMember("minerId", value, d.GetAllocator ());

    value = newBlock.GetParentBlockMinerId ();
    blockInfo.AddMember("parentBlockMinerId", value, d.GetAllocator ());

    value = newBlock.GetBlockSizeBytes ();
    blockInfo.AddMember("size", value, d.GetAllocator ());

    value = newBlock.GetTimeCreated ();
    blockInfo.AddMember("timeCreated", value, d.GetAllocator ());

    value = newBlock.GetTimeReceived ();
    blockInfo.AddMember("timeReceived", value, d.GetAllocator ());

    array.PushBack(blockInfo, d.GetAllocator());
    d.AddMember("blocks", array, d.GetAllocator());
  }

  // Stringify the DOM
  rapidjson::StringBuffer packetInfo;
  rapidjson::Writer<rapidjson::StringBuffer> writer(packetInfo);
  d.Accept(writer);

  for (std::vector<Ipv6Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
  {
    if ( *i != newBlock.GetReceivedFromIpv6Address () )
    {
      const uint8_t delimiter[] = "#";

      m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(packetInfo.GetString()), packetInfo.GetSize(), 0);
	  m_peersSockets[*i]->Send (delimiter, 1, 0);

      if (m_protocolType == STANDARD_PROTOCOL)
        m_nodeStats->invSentBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
      else if (m_protocolType == SENDHEADERS)
        m_nodeStats->headersSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;

      NS_LOG_INFO ("AdvertiseNewBlock: At time " << Simulator::Now ().GetSeconds ()
                   << "s bitcoin node " << GetNode ()->GetId () << " advertised a new Block: "
                   << newBlock << " to " << *i);
    }
  }
}


void
BlockchainNode::AdvertiseFullBlock (const Block &newBlock)
{
  NS_LOG_FUNCTION (this);

  rapidjson::Document d;
  rapidjson::Value value;
  rapidjson::Value array(rapidjson::kArrayType);
  rapidjson::Value blockInfo(rapidjson::kObjectType);
  std::ostringstream stringStream;
  std::string blockHash = stringStream.str();
  d.SetObject();

  value.SetString("block");
  d.AddMember("type", value, d.GetAllocator());

  if (m_protocolType == STANDARD_PROTOCOL)
  {
    value = EXT_INV;
    d.AddMember("message", value, d.GetAllocator());

    stringStream << newBlock.GetBlockHeight () << "/" << newBlock.GetMinerId ();
    blockHash = stringStream.str();
    value.SetString(blockHash.c_str(), blockHash.size(), d.GetAllocator());
    blockInfo.AddMember("hash", value, d.GetAllocator ());

    value = newBlock.GetBlockSizeBytes ();
    blockInfo.AddMember("size", value, d.GetAllocator ());

    value = true;
    blockInfo.AddMember("fullBlock", value, d.GetAllocator ());

    array.PushBack(blockInfo, d.GetAllocator());
    d.AddMember("inv", array, d.GetAllocator());
  }
  else if (m_protocolType == SENDHEADERS)
  {
    rapidjson::Value blockInfo(rapidjson::kObjectType);

    value = newBlock.GetBlockHeight ();
    blockInfo.AddMember("height", value, d.GetAllocator ());

    value = newBlock.GetMinerId ();
    blockInfo.AddMember("minerId", value, d.GetAllocator ());

    value = newBlock.GetParentBlockMinerId ();
    blockInfo.AddMember("parentBlockMinerId", value, d.GetAllocator ());

    value = newBlock.GetBlockSizeBytes ();
    blockInfo.AddMember("size", value, d.GetAllocator ());

    value = newBlock.GetTimeCreated ();
    blockInfo.AddMember("timeCreated", value, d.GetAllocator ());

    value = newBlock.GetTimeReceived ();
    blockInfo.AddMember("timeReceived", value, d.GetAllocator ());

    if (!m_blockTorrent)
    {
      value = HEADERS;
      d.AddMember("message", value, d.GetAllocator());
    }
    else
    {
      value = EXT_HEADERS;
      d.AddMember("message", value, d.GetAllocator());

      value = true;
      blockInfo.AddMember("fullBlock", value, d.GetAllocator ());
    }

    array.PushBack(blockInfo, d.GetAllocator());
    d.AddMember("blocks", array, d.GetAllocator());
  }

  // Stringify the DOM
  rapidjson::StringBuffer packetInfo;
  rapidjson::Writer<rapidjson::StringBuffer> writer(packetInfo);
  d.Accept(writer);

  for (std::vector<Ipv6Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
  {
    const uint8_t delimiter[] = "#";

    m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(packetInfo.GetString()), packetInfo.GetSize(), 0);
    m_peersSockets[*i]->Send (delimiter, 1, 0);

    if (m_protocolType == STANDARD_PROTOCOL)
    {
      m_nodeStats->extInvSentBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
      for (int j=0; j<d["inv"].Size(); j++)
      {
        m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
        if (!d["inv"][j]["fullBlock"].GetBool())
          m_nodeStats->extInvSentBytes += d["inv"][j]["availableChunks"].Size();
      }
    }
    else if (m_protocolType == SENDHEADERS)
    {
      m_nodeStats->extHeadersSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
      for (int j=0; j<d["blocks"].Size(); j++)
      {
        m_nodeStats->extHeadersSentBytes += 1;//fullBlock
        if (!d["blocks"][j]["fullBlock"].GetBool())
          m_nodeStats->extHeadersSentBytes += d["blocks"][j]["availableChunks"].Size()*1;
      }
    }

    NS_LOG_INFO ("AdvertiseFullBlock: At time " << Simulator::Now ().GetSeconds ()
                 << "s bitcoin node " << GetNode ()->GetId () << " advertised a new Block: "
                 << newBlock << " to " << *i);
  }
}



void
BlockchainNode::SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, rapidjson::Document &d, Ptr<Socket> outgoingSocket)
{
  NS_LOG_FUNCTION (this);

  const uint8_t delimiter[] = "#";

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  d["message"].SetInt(responseMessage);
  d.Accept(writer);
  NS_LOG_INFO ("Node " << GetNode ()->GetId () << " got a "
               << getMessageName(receivedMessage) << " message"
               << " and sent a " << getMessageName(responseMessage)
               << " message: " << buffer.GetString());

  outgoingSocket->Send (reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
  outgoingSocket->Send (delimiter, 1, 0);

  switch (d["message"].GetInt())
  {
    case INV:
    {
      m_nodeStats->invSentBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
      break;
    }
    case EXT_INV:
    {
      m_nodeStats->extInvSentBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
      for (int j=0; j<d["inv"].Size(); j++)
      {
        m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
        if (!d["inv"][j]["fullBlock"].GetBool())
          m_nodeStats->extInvSentBytes += d["inv"][j]["availableChunks"].Size();
      }
      break;
    }
    case GET_HEADERS:
    {
      m_nodeStats->getHeadersSentBytes += m_bitcoinMessageHeader + m_getHeadersSizeBytes;
      break;
    }
    case EXT_GET_HEADERS:
    {
      m_nodeStats->extGetHeadersSentBytes += m_bitcoinMessageHeader + m_getHeadersSizeBytes;
      break;
    }
    case HEADERS:
    {
      m_nodeStats->headersSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
      break;
    }
    case EXT_HEADERS:
    {
      m_nodeStats->extHeadersSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
      for (int j=0; j<d["blocks"].Size(); j++)
      {
        m_nodeStats->extHeadersSentBytes += 1;//fullBlock
        if (!d["blocks"][j]["fullBlock"].GetBool())
          m_nodeStats->extHeadersSentBytes += d["blocks"][j]["availableChunks"].Size()*1;
      }
      break;
    }
    case BLOCK:
    {
	  for(int k = 0; k < d["blocks"].Size(); k++)
        m_nodeStats->blockSentBytes += d["blocks"][k]["size"].GetInt();
      m_nodeStats->blockSentBytes += m_bitcoinMessageHeader;
      break;
    }
    case GET_DATA:
    {
      m_nodeStats->getDataSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_inventorySizeBytes;
      break;
    }
    case EXT_GET_DATA:
    {
      m_nodeStats->extGetDataSentBytes += m_bitcoinMessageHeader + m_countBytes + d["chunks"].Size()*m_inventorySizeBytes;
      for (int j=0; j<d["chunks"].Size(); j++)
      {
        m_nodeStats->extGetDataSentBytes += 6; //1Byte(fullBlock) + 4Bytes(numberOfChunks) + 1Byte(requested chunk)
        if (!d["chunks"][j]["fullBlock"].GetBool())
          m_nodeStats->extGetDataSentBytes += d["chunks"][j]["availableChunks"].Size();
      }
      break;
    }
  }
}

void
BlockchainNode::SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, rapidjson::Document &d, Address &outgoingAddress)
{
  NS_LOG_FUNCTION (this);

  const uint8_t delimiter[] = "#";

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  d["message"].SetInt(responseMessage);
  d.Accept(writer);
  NS_LOG_INFO ("Node " << GetNode ()->GetId () << " got a "
               << getMessageName(receivedMessage) << " message"
               << " and sent a " << getMessageName(responseMessage)
               << " message: " << buffer.GetString());

  Ipv6Address outgoingIpv6Address = Inet6SocketAddress::ConvertFrom(outgoingAddress).GetIpv6 ();
  std::map<Ipv6Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv6Address);

  if (it == m_peersSockets.end()) //Create the socket if it doesn't exist
  {
    m_peersSockets[outgoingIpv6Address] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    m_peersSockets[outgoingIpv6Address]->Connect (Inet6SocketAddress (outgoingIpv6Address, m_bitcoinPort));
  }

  m_peersSockets[outgoingIpv6Address]->Send (reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
  m_peersSockets[outgoingIpv6Address]->Send (delimiter, 1, 0);

  switch (d["message"].GetInt())
  {
    case INV:
    {
      m_nodeStats->invSentBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
      break;
    }
    case EXT_INV:
    {
      m_nodeStats->extInvSentBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
      for (int j=0; j<d["inv"].Size(); j++)
      {
        m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
        if (!d["inv"][j]["fullBlock"].GetBool())
          m_nodeStats->extInvSentBytes += d["inv"][j]["availableChunks"].Size();
      }
      break;
    }
    case GET_HEADERS:
    {
      m_nodeStats->getHeadersSentBytes += m_bitcoinMessageHeader + m_getHeadersSizeBytes;
      break;
    }
    case EXT_GET_HEADERS:
    {
      m_nodeStats->extGetHeadersSentBytes += m_bitcoinMessageHeader + m_getHeadersSizeBytes;
      break;
    }
    case HEADERS:
    {
      m_nodeStats->headersSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
      break;
    }
    case EXT_HEADERS:
    {
      m_nodeStats->extHeadersSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
      for (int j=0; j<d["blocks"].Size(); j++)
      {
        m_nodeStats->extHeadersSentBytes += 1;//fullBlock
        if (!d["blocks"][j]["fullBlock"].GetBool())
          m_nodeStats->extHeadersSentBytes += d["blocks"][j]["availableChunks"].Size()*1;
      }
      break;
    }
    case BLOCK:
    {
	  for(int k = 0; k < d["blocks"].Size(); k++)
        m_nodeStats->blockSentBytes += d["blocks"][k]["size"].GetInt();
      m_nodeStats->blockSentBytes += m_bitcoinMessageHeader;
      break;
    }
    case GET_DATA:
    {
      m_nodeStats->getDataSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_inventorySizeBytes;
      break;
    }
    case EXT_GET_DATA:
    {
      m_nodeStats->extGetDataSentBytes += m_bitcoinMessageHeader + m_countBytes + d["chunks"].Size()*m_inventorySizeBytes;
      for (int j=0; j<d["chunks"].Size(); j++)
      {
        m_nodeStats->extGetDataSentBytes += 6; //1Byte(fullBlock) + 4Bytes(numberOfChunks) + 1Byte(requested chunk)
        if (!d["chunks"][j]["fullBlock"].GetBool())
          m_nodeStats->extGetDataSentBytes += d["chunks"][j]["availableChunks"].Size();
      }
      break;
    }
  }
}


void
BlockchainNode::SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, std::string packet, Address &outgoingAddress)
{
  NS_LOG_FUNCTION (this);

  const uint8_t delimiter[] = "#";
  rapidjson::Document d;

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  d.Parse(packet.c_str());
  d["message"].SetInt(responseMessage);
  d.Accept(writer);
  NS_LOG_INFO ("Node " << GetNode ()->GetId () << " got a "
               << getMessageName(receivedMessage) << " message"
               << " and sent a " << getMessageName(responseMessage)
               << " message: " << buffer.GetString());

  Ipv6Address outgoingIpv6Address = Inet6SocketAddress::ConvertFrom(outgoingAddress).GetIpv6 ();
  std::map<Ipv6Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv6Address);

  if (it == m_peersSockets.end()) //Create the socket if it doesn't exist
  {
    m_peersSockets[outgoingIpv6Address] = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    m_peersSockets[outgoingIpv6Address]->Connect (Inet6SocketAddress (outgoingIpv6Address, m_bitcoinPort));
  }

  m_peersSockets[outgoingIpv6Address]->Send (reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
  m_peersSockets[outgoingIpv6Address]->Send (delimiter, 1, 0);


  switch (d["message"].GetInt())
  {
    case INV:
    {
      m_nodeStats->invSentBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
      break;
    }
    case EXT_INV:
    {
      m_nodeStats->extInvSentBytes += m_bitcoinMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
      for (int j=0; j<d["inv"].Size(); j++)
      {
        m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
        if (!d["inv"][j]["fullBlock"].GetBool())
          m_nodeStats->extInvSentBytes += d["inv"][j]["availableChunks"].Size();
      }
      break;
    }
    case GET_HEADERS:
    {
      m_nodeStats->getHeadersSentBytes += m_bitcoinMessageHeader + m_getHeadersSizeBytes;
      break;
    }
    case EXT_GET_HEADERS:
    {
      m_nodeStats->extGetHeadersSentBytes += m_bitcoinMessageHeader + m_getHeadersSizeBytes;
      break;
    }
    case HEADERS:
    {
      m_nodeStats->headersSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
      break;
    }
    case EXT_HEADERS:
    {
      m_nodeStats->extHeadersSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
      for (int j=0; j<d["blocks"].Size(); j++)
      {
        m_nodeStats->extHeadersSentBytes += 1;//fullBlock
        if (!d["blocks"][j]["fullBlock"].GetBool())
          m_nodeStats->extHeadersSentBytes += d["blocks"][j]["availableChunks"].Size()*1;
      }
      break;
    }
    case BLOCK:
    {
	  for(int k = 0; k < d["blocks"].Size(); k++)
        m_nodeStats->blockSentBytes += d["blocks"][k]["size"].GetInt();
      m_nodeStats->blockSentBytes += m_bitcoinMessageHeader;
      break;
    }
    case GET_DATA:
    {
      m_nodeStats->getDataSentBytes += m_bitcoinMessageHeader + m_countBytes + d["blocks"].Size()*m_inventorySizeBytes;
      break;
    }
    case EXT_GET_DATA:
    {
      m_nodeStats->extGetDataSentBytes += m_bitcoinMessageHeader + m_countBytes + d["chunks"].Size()*m_inventorySizeBytes;
      for (int j=0; j<d["chunks"].Size(); j++)
      {
        m_nodeStats->extGetDataSentBytes += 6; //1Byte(fullBlock) + 4Bytes(numberOfChunks) + 1Byte(requested chunk)
        if (!d["chunks"][j]["fullBlock"].GetBool())
          m_nodeStats->extGetDataSentBytes += d["chunks"][j]["availableChunks"].Size();
      }
      break;
    }
  }
}


void
BlockchainNode::PrintQueueInv()
{
  NS_LOG_FUNCTION (this);

  std::cout << "Node " <<  GetNode()->GetId() << ": The queueINV is:\n";

  for(auto &elem : m_queueInv)
  {
    std::vector<Address>::iterator  block_it;
    std::cout << "  " << elem.first << ":";

    for (block_it = elem.second.begin();  block_it < elem.second.end(); block_it++)
    {
      std::cout << " " << Inet6SocketAddress::ConvertFrom(*block_it).GetIpv6 ();
    }
    std::cout << "\n";
  }
  std::cout << std::endl;
}


void
BlockchainNode::PrintInvTimeouts()
{
  NS_LOG_FUNCTION (this);

  std::cout << "Node " <<  GetNode()->GetId() << ": The m_invTimeouts is:\n";

  for(auto &elem : m_invTimeouts)
  {
    std::cout << "  " << elem.first << ":\n";
  }
  std::cout << std::endl;
}

void
BlockchainNode::PrintOnlyHeadersReceived()
{
  NS_LOG_FUNCTION (this);

  std::cout << "Node " <<  GetNode()->GetId() << ": The m_onlyHeadersReceived is:\n";

  for(auto &elem : m_onlyHeadersReceived)
  {
    std::vector<Address>::iterator  block_it;
    std::cout << "  " << elem.first << ": " << elem.second << "\n";
  }
  std::cout << std::endl;
}


void
BlockchainNode::InvTimeoutExpired(std::string blockHash)
{
  NS_LOG_FUNCTION (this);

  std::string   invDelimiter = "/";
  size_t        invPos = blockHash.find(invDelimiter);

  int height = atoi(blockHash.substr(0, invPos).c_str());
  int minerId = atoi(blockHash.substr(invPos+1, blockHash.size()).c_str());

  NS_LOG_INFO ("Node " << GetNode ()->GetId () << ": At time "  << Simulator::Now ().GetSeconds ()
                << " the timeout for block " << blockHash << " expired");

  m_nodeStats->blockTimeouts ++;
  //PrintQueueInv();
  //PrintInvTimeouts();

  m_queueInv[blockHash].erase(m_queueInv[blockHash].begin());
  m_invTimeouts.erase(blockHash);

  //PrintQueueInv();
  //PrintInvTimeouts();

  if (!m_queueInv[blockHash].empty() && !m_blockchain.HasBlock(height, minerId) && !m_blockchain.IsOrphan(height, minerId) && !ReceivedButNotValidated(blockHash))
  {
    rapidjson::Document   d;
    EventId               timeout;
    rapidjson::Value      value(INV);
    rapidjson::Value      array(rapidjson::kArrayType);

    d.SetObject();

    d.AddMember("message", value, d.GetAllocator());

    value.SetString("block");
    d.AddMember("type", value, d.GetAllocator());

    value.SetString(blockHash.c_str(), blockHash.size(), d.GetAllocator());
    array.PushBack(value, d.GetAllocator());
    d.AddMember("blocks", array, d.GetAllocator());

    int index = rand() % m_queueInv[blockHash].size();
    Address temp = m_queueInv[blockHash][0];
    m_queueInv[blockHash][0] = m_queueInv[blockHash][index];
    m_queueInv[blockHash][index] = temp;

    SendMessage(INV, GET_HEADERS, d, *(m_queueInv[blockHash].begin()));
    SendMessage(INV, GET_DATA, d, *(m_queueInv[blockHash].begin()));

    timeout = Simulator::Schedule (m_invTimeoutMinutes, &BlockchainNode::InvTimeoutExpired, this, blockHash);
    m_invTimeouts[blockHash] = timeout;
  }
  else
    m_queueInv.erase(blockHash);

  //PrintQueueInv();
  //PrintInvTimeouts();
}


bool
BlockchainNode::ReceivedButNotValidated (std::string blockHash)
{
  NS_LOG_FUNCTION (this);

  if ( m_receivedNotValidated.find(blockHash) != m_receivedNotValidated.end() )
    return true;
  else
    return false;
}


void
BlockchainNode::RemoveReceivedButNotValidated (std::string blockHash)
{
  NS_LOG_FUNCTION (this);


  if ( m_receivedNotValidated.find(blockHash) != m_receivedNotValidated.end() )
  {
    m_receivedNotValidated.erase(blockHash);
  }
  else
  {
    NS_LOG_WARN (blockHash << " was not found in m_receivedNotValidated");
  }
}


bool
BlockchainNode::OnlyHeadersReceived (std::string blockHash)
{
  NS_LOG_FUNCTION (this);

  if (m_onlyHeadersReceived.find(blockHash) != m_onlyHeadersReceived.end())
    return true;
  else
    return false;
}



void
BlockchainNode::RemoveSendTime ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("RemoveSendTime: At Time " << Simulator::Now ().GetSeconds () << " " << m_sendBlockTimes.front() << " was removed");
  m_sendBlockTimes.erase(m_sendBlockTimes.begin());
}


void
BlockchainNode::RemoveCompressedBlockSendTime ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("RemoveCompressedBlockSendTime: At Time " << Simulator::Now ().GetSeconds () << " " << m_sendCompressedBlockTimes.front() << " was removed");
  m_sendCompressedBlockTimes.erase(m_sendCompressedBlockTimes.begin());
}


void
BlockchainNode::RemoveReceiveTime ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("RemoveReceiveTime: At Time " << Simulator::Now ().GetSeconds () << " " << m_receiveBlockTimes.front() << " was removed");
  m_receiveBlockTimes.erase(m_receiveBlockTimes.begin());
}


void
BlockchainNode::RemoveCompressedBlockReceiveTime ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("RemoveCompressedBlockReceiveTime: At Time " << Simulator::Now ().GetSeconds () << " " << m_receiveCompressedBlockTimes.front() << " was removed");
  m_receiveCompressedBlockTimes.erase(m_receiveCompressedBlockTimes.begin());
}


void
BlockchainNode::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void BlockchainNode::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


void
BlockchainNode::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&BlockchainNode::HandleRead, this));
}


} // Namespace ns3
