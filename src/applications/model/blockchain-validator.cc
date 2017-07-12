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
#include "ns3/blockchain-validator.h"
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"
#include <fstream>
#include <time.h>
#include <sys/time.h>


static double GetWallTime();

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BlockchainValidator");

NS_OBJECT_ENSURE_REGISTERED (BlockchainValidator);

TypeId
BlockchainValidator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BlockchainValidator")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BlockchainValidator> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&BlockchainValidator::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&BlockchainValidator::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("BlockTorrent",
                   "Enable the BlockTorrent protocol",
                   BooleanValue (false),
                   MakeBooleanAccessor (&BlockchainValidator::m_blockTorrent),
                   MakeBooleanChecker ())
    .AddAttribute ("SPV",
                   "Enable SPV Mechanism",
                   BooleanValue (false),
                   MakeBooleanAccessor (&BlockchainValidator::m_spv),
                   MakeBooleanChecker ())
    .AddAttribute ("NumberOfMiners",
                   "The number of miners",
                   UintegerValue (16),
                   MakeUintegerAccessor (&BlockchainValidator::m_noMiners),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FixedBlockSize",
                   "The fixed size of the block",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BlockchainValidator::m_fixedBlockSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FixedBlockIntervalGeneration",
                   "The fixed time to wait between two consecutive block generations",
                   DoubleValue (0),
                   MakeDoubleAccessor (&BlockchainValidator::m_fixedBlockTimeGeneration),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("InvTimeoutMinutes",
                   "The timeout of inv messages in minutes",
                   TimeValue (Minutes (20)),
                   MakeTimeAccessor (&BlockchainValidator::m_invTimeoutMinutes),
                   MakeTimeChecker())
    .AddAttribute ("HashRate",
                   "The hash rate of the miner",
                   DoubleValue (0.2),
                   MakeDoubleAccessor (&BlockchainValidator::m_hashRate),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("BlockGenBinSize",
                   "The block generation bin size",
                   DoubleValue (-1),
                   MakeDoubleAccessor (&BlockchainValidator::m_blockGenBinSize),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("BlockGenParameter",
                   "The block generation distribution parameter",
                   DoubleValue (-1),
                   MakeDoubleAccessor (&BlockchainValidator::m_blockGenParameter),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("AverageBlockGenIntervalSeconds",
                   "The average block generation interval we aim at (in seconds)",
                   DoubleValue (10*60),
                   MakeDoubleAccessor (&BlockchainValidator::m_averageBlockGenIntervalSeconds),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ChunkSize",
                   "The fixed size of the block chunk",
                   UintegerValue (100000),
                   MakeUintegerAccessor (&BlockchainValidator::m_chunkSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&BlockchainValidator::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
  ;
  return tid;
}

BlockchainValidator::BlockchainValidator () : BlockchainNode(), m_realAverageBlockGenIntervalSeconds(10*m_secondsPerMin),
                                m_timeStart (0), m_timeFinish (0), m_fistToMine (false)
{
  NS_LOG_FUNCTION (this);
  m_minerAverageBlockGenInterval = 0;
  m_minerGeneratedBlocks = 0;
  m_previousBlockGenerationTime = 0;

  std::random_device rd;
  m_generator.seed(rd());

  if (m_fixedBlockTimeGeneration > 0)
    m_nextBlockTime = m_fixedBlockTimeGeneration;
  else
    m_nextBlockTime = 0;

  if (m_fixedBlockSize > 0)
    m_nextBlockSize = m_fixedBlockSize;
  else
    m_nextBlockSize = 0;

  m_isMiner = true;
}


BlockchainValidator::~BlockchainValidator(void)
{
  NS_LOG_FUNCTION (this);
}


void
BlockchainValidator::StartApplication ()    // Called at time specified by Start
{
  BlockchainNode::StartApplication ();
  NS_LOG_WARN ("Validator " << GetNode()->GetId() << " m_noMiners = " << m_noMiners << "");
  NS_LOG_WARN ("Validator " << GetNode()->GetId() << " m_realAverageBlockGenIntervalSeconds = " << m_realAverageBlockGenIntervalSeconds << "s");
  NS_LOG_WARN ("Validator " << GetNode()->GetId() << " m_averageBlockGenIntervalSeconds = " << m_averageBlockGenIntervalSeconds << "s");
  NS_LOG_WARN ("Validator " << GetNode()->GetId() << " m_fixedBlockTimeGeneration = " << m_fixedBlockTimeGeneration << "s");
  NS_LOG_WARN ("Validator " << GetNode()->GetId() << " m_hashRate = " << m_hashRate );
  NS_LOG_WARN ("Validator " << GetNode()->GetId() << " m_blockBroadcastType = " << getBlockBroadcastType(m_blockBroadcastType));

  if (m_blockGenBinSize < 0 && m_blockGenParameter < 0)
  {
    m_blockGenBinSize = 1./m_secondsPerMin/1000;
    m_blockGenParameter = 0.19 * m_blockGenBinSize / 2;
  }
  else
    m_blockGenParameter *= m_hashRate;

  if (m_fixedBlockTimeGeneration == 0)
    m_blockGenTimeDistribution.param(std::geometric_distribution<int>::param_type(m_blockGenParameter));

    m_nextBlockSize = m_fixedBlockSize;

/*   if (GetNode()->GetId() == 0)
  {
    Block newBlock(1, 0, -1, 500000, 0, 0, Ipv6Address("0.0.0.0"));
    m_blockchain.AddBlock(newBlock);
  } */

  // m_nodeStats->hashRate = m_hashRate;
  // m_nodeStats->miner = 1;

  ScheduleNextMiningEvent ();
}

void
BlockchainValidator::StopApplication ()
{
  BlockchainNode::StopApplication ();
  Simulator::Cancel (m_nextMiningEvent);

  NS_LOG_WARN ("The miner " << GetNode ()->GetId () << " with hash rate = " << m_hashRate << " generated " << m_minerGeneratedBlocks
                << " blocks "<< "(" << 100. * m_minerGeneratedBlocks / (m_blockchain.GetTotalBlocks() - 1)
                << "%) with average block generation time = " << m_minerAverageBlockGenInterval
                << "s or " << static_cast<int>(m_minerAverageBlockGenInterval) / m_secondsPerMin << "min and "
                << m_minerAverageBlockGenInterval - static_cast<int>(m_minerAverageBlockGenInterval) / m_secondsPerMin * m_secondsPerMin << "s"
                << " and average size " << m_minerAverageBlockSize << " Bytes");


  m_nodeStats->minerGeneratedBlocks = m_minerGeneratedBlocks;
  m_nodeStats->minerAverageBlockGenInterval = m_minerAverageBlockGenInterval;
  m_nodeStats->minerAverageBlockSize = m_minerAverageBlockSize;

  if (m_fistToMine)
  {
    m_timeFinish = GetWallTime();
    std::cout << "Time/Block = " << (m_timeFinish - m_timeStart) / (m_blockchain.GetTotalBlocks() - 1) << "s\n";
  }
}

void
BlockchainValidator::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  BlockchainNode::DoDispose ();
}

double
BlockchainValidator::GetFixedBlockTimeGeneration(void) const
{
  NS_LOG_FUNCTION (this);
  return m_fixedBlockTimeGeneration;
}

void
BlockchainValidator::SetFixedBlockTimeGeneration(double fixedBlockTimeGeneration)
{
  NS_LOG_FUNCTION (this);
  m_fixedBlockTimeGeneration = fixedBlockTimeGeneration;
}

uint32_t
BlockchainValidator::GetFixedBlockSize(void) const
{
  NS_LOG_FUNCTION (this);
  return m_fixedBlockSize;
}

void
BlockchainValidator::SetFixedBlockSize(uint32_t fixedBlockSize)
{
  NS_LOG_FUNCTION (this);
  m_fixedBlockSize = fixedBlockSize;
}

double
BlockchainValidator::GetBlockGenBinSize(void) const
{
  NS_LOG_FUNCTION (this);
  return m_blockGenBinSize;
}

void
BlockchainValidator::SetBlockGenBinSize (double blockGenBinSize)
{
  NS_LOG_FUNCTION (this);
  m_blockGenBinSize = blockGenBinSize;
}

double
BlockchainValidator::GetBlockGenParameter(void) const
{
  NS_LOG_FUNCTION (this);
  return m_blockGenParameter;
}

void
BlockchainValidator::SetBlockGenParameter (double blockGenParameter)
{
  NS_LOG_FUNCTION (this);
  m_blockGenParameter = blockGenParameter;
  m_blockGenTimeDistribution.param(std::geometric_distribution<int>::param_type(m_blockGenParameter));

}

double
BlockchainValidator::GetHashRate(void) const
{
  NS_LOG_FUNCTION (this);
  return m_hashRate;
}

void
BlockchainValidator::SetHashRate (double hashRate)
{
  NS_LOG_FUNCTION (this);
  m_hashRate = hashRate;
}

void
BlockchainValidator::SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType)
{
  NS_LOG_FUNCTION (this);
  m_blockBroadcastType = blockBroadcastType;
}

void
BlockchainValidator::ScheduleNextMiningEvent (void)
{
  NS_LOG_FUNCTION (this);

  if(m_fixedBlockTimeGeneration > 0)
  {
    m_nextBlockTime = m_fixedBlockTimeGeneration;

    NS_LOG_DEBUG ("Time " << Simulator::Now ().GetSeconds () << ": Miner " << GetNode ()->GetId ()
                << " fixed Block Time Generation " << m_fixedBlockTimeGeneration << "s");
    m_nextMiningEvent = Simulator::Schedule (Seconds(m_fixedBlockTimeGeneration), &BlockchainValidator::ValidateBlock, this);
  }
  else
  {
    m_nextBlockTime = m_blockGenTimeDistribution(m_generator)*m_blockGenBinSize*m_secondsPerMin
                    *( m_averageBlockGenIntervalSeconds/m_realAverageBlockGenIntervalSeconds )/m_hashRate;

    //NS_LOG_DEBUG("m_nextBlockTime = " << m_nextBlockTime << ", binsize = " << m_blockGenBinSize << ", m_blockGenParameter = " << m_blockGenParameter << ", hashrate = " << m_hashRate);
    m_nextMiningEvent = Simulator::Schedule (Seconds(m_nextBlockTime), &BlockchainValidator::ValidateBlock, this);

    NS_LOG_WARN ("Time " << Simulator::Now ().GetSeconds () << ": Miner " << GetNode ()->GetId () << " will generate a block in "
                 << m_nextBlockTime << "s or " << static_cast<int>(m_nextBlockTime) / m_secondsPerMin
                 << "  min and  " << static_cast<int>(m_nextBlockTime) % m_secondsPerMin
                 << "s using Geometric Block Time Generation with parameter = "<< m_blockGenParameter);
  }
}

void
BlockchainValidator::ValidateBlock (void)
{
  NS_LOG_FUNCTION (this);
  rapidjson::Document inv;
  rapidjson::Document block;

  int height =  m_blockchain.GetCurrentTopBlock()->GetBlockHeight() + 1;
  int minerId = GetNode ()->GetId ();
  int parentBlockMinerId = m_blockchain.GetCurrentTopBlock()->GetMinerId();
  double currentTime = Simulator::Now ().GetSeconds ();
  std::ostringstream stringStream;
  std::string blockHash;

  stringStream << height << "/" << minerId;
  blockHash = stringStream.str();

  inv.SetObject();
  block.SetObject();

  if (height == 1)
  {
    m_fistToMine = true;
	m_timeStart = GetWallTime();
  }
/*   //For attacks
   if (GetNode ()->GetId () == 0)
     height = 2 - m_minerGeneratedBlocks;

   if (GetNode ()->GetId () == 0)
   {
	if (height == 1)
      parentBlockMinerId = -1;
    else
	  parentBlockMinerId = 0;
   } */


  if (m_fixedBlockSize > 0)
    m_nextBlockSize = m_fixedBlockSize;
  else
  {
    m_nextBlockSize = m_blockSizeDistribution(m_generator) * 1000;	// *1000 because the m_blockSizeDistribution returns KBytes

    m_nextBlockSize = m_nextBlockSize*m_averageBlockGenIntervalSeconds / m_realAverageBlockGenIntervalSeconds;
  }

  if (m_nextBlockSize < m_averageTransactionSize)
    m_nextBlockSize = m_averageTransactionSize + m_headersSizeBytes;

    blockDataTuple blankBlockData = {0, 0, Ipv6Address("0::0::0::0"), "" ,""};
    std::map<int, blockDataTuple> dataMap;
    dataMap[0] = blankBlockData;
  Block newBlock (height, minerId, parentBlockMinerId, m_nextBlockSize,
                  currentTime, dataMap, currentTime, Ipv6Address("0::0::0::1"));

  switch(m_blockBroadcastType)
  {
    case STANDARD:
    {
      rapidjson::Value value;
      rapidjson::Value array(rapidjson::kArrayType);
      rapidjson::Value blockInfo(rapidjson::kObjectType);

      value.SetString("block"); //Remove
      inv.AddMember("type", value, inv.GetAllocator());

      if (m_protocolType == STANDARD_PROTOCOL)
      {
        if (!m_blockTorrent)
        {
          value = INV;
          inv.AddMember("message", value, inv.GetAllocator());

          value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
          array.PushBack(value, inv.GetAllocator());

          inv.AddMember("inv", array, inv.GetAllocator());
        }
        else
        {
          value = EXT_INV;
          inv.AddMember("message", value, inv.GetAllocator());

          value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
          blockInfo.AddMember("hash", value, inv.GetAllocator ());

	      value = newBlock.GetBlockSizeBytes ();
          blockInfo.AddMember("size", value, inv.GetAllocator ());

          value = true;
          blockInfo.AddMember("fullBlock", value, inv.GetAllocator ());

          array.PushBack(blockInfo, inv.GetAllocator());
          inv.AddMember("inv", array, inv.GetAllocator());
        }
      }
      else if (m_protocolType == SENDHEADERS)
      {

        value = newBlock.GetBlockHeight ();
        blockInfo.AddMember("height", value, inv.GetAllocator ());

        value = newBlock.GetMinerId ();
        blockInfo.AddMember("minerId", value, inv.GetAllocator ());

        value = newBlock.GetParentBlockMinerId ();
        blockInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

        value = newBlock.GetBlockSizeBytes ();
        blockInfo.AddMember("size", value, inv.GetAllocator ());

        value = newBlock.GetTimeCreated ();
        blockInfo.AddMember("timeCreated", value, inv.GetAllocator ());

        value = newBlock.GetTimeReceived ();
        blockInfo.AddMember("timeReceived", value, inv.GetAllocator ());

        if (!m_blockTorrent)
        {
          value = HEADERS;
          inv.AddMember("message", value, inv.GetAllocator());
        }
        else
        {
          value = EXT_HEADERS;
          inv.AddMember("message", value, inv.GetAllocator());

          value = true;
          blockInfo.AddMember("fullBlock", value, inv.GetAllocator ());
        }

        array.PushBack(blockInfo, inv.GetAllocator());
        inv.AddMember("blocks", array, inv.GetAllocator());
      }
      break;
    }
    case UNSOLICITED:
    {
      rapidjson::Value value (BLOCK);
      rapidjson::Value blockInfo(rapidjson::kObjectType);
      rapidjson::Value array(rapidjson::kArrayType);

      block.AddMember("message", value, block.GetAllocator());

      value.SetString("block"); //Remove
      block.AddMember("type", value, block.GetAllocator());

      value = newBlock.GetBlockHeight ();
      blockInfo.AddMember("height", value, block.GetAllocator ());

      value = newBlock.GetMinerId ();
      blockInfo.AddMember("minerId", value, block.GetAllocator ());

      value = newBlock.GetParentBlockMinerId ();
      blockInfo.AddMember("parentBlockMinerId", value, block.GetAllocator ());

      value = newBlock.GetBlockSizeBytes ();
      blockInfo.AddMember("size", value, block.GetAllocator ());

      value = newBlock.GetTimeCreated ();
      blockInfo.AddMember("timeCreated", value, block.GetAllocator ());

      value = newBlock.GetTimeReceived ();
      blockInfo.AddMember("timeReceived", value, block.GetAllocator ());

      array.PushBack(blockInfo, block.GetAllocator());
      block.AddMember("blocks", array, block.GetAllocator());

      break;
    }
    case RELAY_NETWORK:
    {
      rapidjson::Value value;
      rapidjson::Value headersInfo(rapidjson::kObjectType);
      rapidjson::Value chunkInfo(rapidjson::kObjectType);
      rapidjson::Value blockInfo(rapidjson::kObjectType);
      rapidjson::Value invArray(rapidjson::kArrayType);
      rapidjson::Value blockArray(rapidjson::kArrayType);

      value.SetString("block"); //Remove
      inv.AddMember("type", value, inv.GetAllocator());

      if (m_protocolType == STANDARD_PROTOCOL)
      {
        if (!m_blockTorrent)
        {
          value = INV;
          inv.AddMember("message", value, inv.GetAllocator());

          value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
          invArray.PushBack(value, inv.GetAllocator());

          inv.AddMember("inv", invArray, inv.GetAllocator());
        }
        else
        {
          value = EXT_INV;
          inv.AddMember("message", value, inv.GetAllocator());

          value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
          chunkInfo.AddMember("hash", value, inv.GetAllocator ());

          value = newBlock.GetBlockSizeBytes ();
          chunkInfo.AddMember("size", value, inv.GetAllocator ());

          value = true;
          chunkInfo.AddMember("fullBlock", value, inv.GetAllocator ());

          invArray.PushBack(chunkInfo, inv.GetAllocator());
          inv.AddMember("inv", invArray, inv.GetAllocator());
        }
      }
      else if (m_protocolType == SENDHEADERS)
      {

        value = newBlock.GetBlockHeight ();
        headersInfo.AddMember("height", value, inv.GetAllocator ());

        value = newBlock.GetMinerId ();
        headersInfo.AddMember("minerId", value, inv.GetAllocator ());

        value = newBlock.GetParentBlockMinerId ();
        headersInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

        value = newBlock.GetBlockSizeBytes ();
        headersInfo.AddMember("size", value, inv.GetAllocator ());

        value = newBlock.GetTimeCreated ();
        headersInfo.AddMember("timeCreated", value, inv.GetAllocator ());

        value = newBlock.GetTimeReceived ();
        headersInfo.AddMember("timeReceived", value, inv.GetAllocator ());

        if (!m_blockTorrent)
        {
          value = HEADERS;
          inv.AddMember("message", value, inv.GetAllocator());
        }
        else
        {
          value = EXT_HEADERS;
          inv.AddMember("message", value, inv.GetAllocator());

          value = true;
          headersInfo.AddMember("fullBlock", value, inv.GetAllocator ());
        }

        invArray.PushBack(headersInfo, inv.GetAllocator());
        inv.AddMember("blocks", invArray, inv.GetAllocator());
      }



      //Unsolicited for validators
      value = BLOCK;
      block.AddMember("message", value, block.GetAllocator());

      value.SetString("compressed-block"); //Remove
      block.AddMember("type", value, block.GetAllocator());

      value = newBlock.GetBlockHeight ();
      blockInfo.AddMember("height", value, block.GetAllocator ());

      value = newBlock.GetMinerId ();
      blockInfo.AddMember("minerId", value, block.GetAllocator ());

      value = newBlock.GetParentBlockMinerId ();
      blockInfo.AddMember("parentBlockMinerId", value, block.GetAllocator ());

      value = newBlock.GetBlockSizeBytes ();
      blockInfo.AddMember("size", value, block.GetAllocator ());

      value = newBlock.GetTimeCreated ();
      blockInfo.AddMember("timeCreated", value, block.GetAllocator ());

      value = newBlock.GetTimeReceived ();
      blockInfo.AddMember("timeReceived", value, block.GetAllocator ());

      blockArray.PushBack(blockInfo, block.GetAllocator());
      block.AddMember("blocks", blockArray, block.GetAllocator());

      break;
    }
    case UNSOLICITED_RELAY_NETWORK:
    {
      rapidjson::Value value;
      rapidjson::Value blockNodesInfo(rapidjson::kObjectType);
      rapidjson::Value blockInfo(rapidjson::kObjectType);
      rapidjson::Value invArray(rapidjson::kArrayType);
      rapidjson::Value blockArray(rapidjson::kArrayType);

      //Unsolicited for nodes
      value = BLOCK;
      inv.AddMember("message", value, inv.GetAllocator());

      value.SetString("block"); //Remove
      inv.AddMember("type", value, inv.GetAllocator());

      value = newBlock.GetBlockHeight ();
      blockNodesInfo.AddMember("height", value, inv.GetAllocator ());

      value = newBlock.GetMinerId ();
      blockNodesInfo.AddMember("minerId", value, inv.GetAllocator ());

      value = newBlock.GetParentBlockMinerId ();
      blockNodesInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

      value = newBlock.GetBlockSizeBytes ();
      blockNodesInfo.AddMember("size", value, inv.GetAllocator ());

      value = newBlock.GetTimeCreated ();
      blockNodesInfo.AddMember("timeCreated", value, inv.GetAllocator ());

      value = newBlock.GetTimeReceived ();
      blockNodesInfo.AddMember("timeReceived", value, inv.GetAllocator ());

      invArray.PushBack(blockNodesInfo, inv.GetAllocator());
      inv.AddMember("blocks", invArray, inv.GetAllocator());


      //Unsolicited for validators
      value = BLOCK;
      block.AddMember("message", value, block.GetAllocator());

      value.SetString("compressed-block"); //Remove
      block.AddMember("type", value, block.GetAllocator());

      value = newBlock.GetBlockHeight ();
      blockInfo.AddMember("height", value, block.GetAllocator ());

      value = newBlock.GetMinerId ();
      blockInfo.AddMember("minerId", value, block.GetAllocator ());

      value = newBlock.GetParentBlockMinerId ();
      blockInfo.AddMember("parentBlockMinerId", value, block.GetAllocator ());

      value = newBlock.GetBlockSizeBytes ();
      blockInfo.AddMember("size", value, block.GetAllocator ());

      value = newBlock.GetTimeCreated ();
      blockInfo.AddMember("timeCreated", value, block.GetAllocator ());

      value = newBlock.GetTimeReceived ();
      blockInfo.AddMember("timeReceived", value, block.GetAllocator ());

      blockArray.PushBack(blockInfo, block.GetAllocator());
      block.AddMember("blocks", blockArray, block.GetAllocator());

      break;
    }
  }


  /**
   * Update m_meanBlockReceiveTime with the timeCreated of the newly generated block
   */
  m_meanBlockReceiveTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockReceiveTime
                         + (currentTime - m_previousBlockReceiveTime)/(m_blockchain.GetTotalBlocks());
  m_previousBlockReceiveTime = currentTime;

  m_meanBlockPropagationTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockPropagationTime;

  m_meanBlockSize = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockSize
                  + (m_nextBlockSize)/static_cast<double>(m_blockchain.GetTotalBlocks());

  m_blockchain.AddBlock(newBlock);

  // Stringify the DOM
  rapidjson::StringBuffer invInfo;
  rapidjson::Writer<rapidjson::StringBuffer> invWriter(invInfo);
  inv.Accept(invWriter);

  rapidjson::StringBuffer blockInfo;
  rapidjson::Writer<rapidjson::StringBuffer> blockWriter(blockInfo);
  block.Accept(blockWriter);

  int count = 0;

  for (std::vector<Ipv6Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i, ++count)
  {

    const uint8_t delimiter[] = "#";

    switch(m_blockBroadcastType)
    {
      case STANDARD:
      {
        m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
        m_peersSockets[*i]->Send (delimiter, 1, 0);

        if (m_protocolType == STANDARD_PROTOCOL && !m_blockTorrent)
          m_nodeStats->invSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
        else if (m_protocolType == SENDHEADERS && !m_blockTorrent)
          m_nodeStats->headersSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
        else if (m_protocolType == STANDARD_PROTOCOL && m_blockTorrent)
        {
          m_nodeStats->extInvSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
          for (int j=0; j<inv["inv"].Size(); j++)
          {
            m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
            if (!inv["inv"][j]["fullBlock"].GetBool())
              m_nodeStats->extInvSentBytes += inv["inv"][j]["availableChunks"].Size()*1;
          }
        }
        else if (m_protocolType == SENDHEADERS && m_blockTorrent)
        {
          m_nodeStats->extHeadersSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
          for (int j=0; j<inv["blocks"].Size(); j++)
          {
            m_nodeStats->extHeadersSentBytes += 1;//fullBlock
            if (!inv["blocks"][j]["fullBlock"].GetBool())
              m_nodeStats->extHeadersSentBytes += inv["inv"][j]["availableChunks"].Size();
          }
        }

        NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                     << "s bitcoin miner " << GetNode ()->GetId ()
                     << " sent a packet " << invInfo.GetString()
			         << " to " << *i);
        break;
      }
      case UNSOLICITED:
      {
        m_nodeStats->blockSentBytes += m_bitcoinMessageHeader + block["blocks"][0]["size"].GetInt();

        double sendTime = m_nextBlockSize / m_uploadSpeed;
        double eventTime;

/*                 std::cout << "Node " << GetNode()->GetId() << "-" << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                          << " " << m_peersDownloadSpeeds[Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()] << " Mbps , time = "
                          << Simulator::Now ().GetSeconds() << "s \n"; */

        if (m_sendBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendBlockTimes.back())
        {
          eventTime = 0;
        }
        else
        {
          //std::cout << "m_sendBlockTimes.back() = m_sendBlockTimes.back() = " << m_sendBlockTimes.back() << std::endl;
          eventTime = m_sendBlockTimes.back() - Simulator::Now ().GetSeconds();
        }
        m_sendBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);


        /* std::cout << sendTime << " " << eventTime << " " << m_sendBlockTimes.size() << std::endl; */
        NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << *i
                    << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");

        std::string packet = blockInfo.GetString();
        Simulator::Schedule (Seconds(eventTime), &BlockchainValidator::SendBlock, this, packet, m_peersSockets[*i]);
        Simulator::Schedule (Seconds(eventTime + sendTime), &BlockchainValidator::RemoveSendTime, this);

        break;
      }
      case RELAY_NETWORK:
      {
        if(count < m_noMiners - 1)
        {
          int    noTransactions = static_cast<int>((m_nextBlockSize - m_blockHeadersSizeBytes)/m_averageTransactionSize);
          long   blockSize = m_blockHeadersSizeBytes + m_transactionIndexSize*noTransactions;
          double sendTime = blockSize / m_uploadSpeed;
          double eventTime;

          m_nodeStats->blockSentBytes += m_bitcoinMessageHeader + blockSize;

/* 				std::cout << "Node " << GetNode()->GetId() << "-" << *i
                            << " " << m_peersDownloadSpeeds[*i] << " Mbps , time = "
                            << Simulator::Now ().GetSeconds() << "s \n"; */

          if (m_sendCompressedBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendCompressedBlockTimes.back())
          {
            eventTime = 0;
          }
          else
          {
            //std::cout << "m_sendCompressedBlockTimes.back() = m_sendCompressedBlockTimes.back() = " << m_sendCompressedBlockTimes.back() << std::endl;
            eventTime = m_sendCompressedBlockTimes.back() - Simulator::Now ().GetSeconds();
          }
          m_sendCompressedBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);

          //std::cout << sendTime << " " << eventTime << " " << m_sendCompressedBlockTimes.size() << std::endl;
          NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << *i
                      << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");

          //sendTime = blockSize / m_uploadSpeed * count;
          //std::cout << sendTime << std::endl;

          std::string packet = blockInfo.GetString();
          Simulator::Schedule (Seconds(sendTime), &BlockchainValidator::SendBlock, this, packet, m_peersSockets[*i]);
          Simulator::Schedule (Seconds(eventTime + sendTime), &BlockchainValidator::RemoveCompressedBlockSendTime, this);

        }
        else
        {
          m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
          m_peersSockets[*i]->Send (delimiter, 1, 0);

          if (m_protocolType == STANDARD_PROTOCOL && !m_blockTorrent)
            m_nodeStats->invSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
          else if (m_protocolType == SENDHEADERS && !m_blockTorrent)
            m_nodeStats->headersSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
          else if (m_protocolType == STANDARD_PROTOCOL && m_blockTorrent)
          {
            m_nodeStats->extInvSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
            for (int j=0; j<inv["inv"].Size(); j++)
            {
              m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
              if (!inv["inv"][j]["fullBlock"].GetBool())
                m_nodeStats->extInvSentBytes += inv["inv"][j]["availableChunks"].Size()*1;
            }
          }
          else if (m_protocolType == SENDHEADERS && m_blockTorrent)
          {
            m_nodeStats->extHeadersSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
            for (int j=0; j<inv["blocks"].Size(); j++)
            {
            m_nodeStats->extHeadersSentBytes += 1;//fullBlock
            if (!inv["blocks"][j]["fullBlock"].GetBool())
                m_nodeStats->extHeadersSentBytes += inv["blocks"][j]["availableChunks"].Size()*1;
            }
          }

          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s bitcoin miner " << GetNode ()->GetId ()
                       << " sent a packet " << invInfo.GetString()
                       << " to " << *i);
        }
        break;
      }
      case UNSOLICITED_RELAY_NETWORK:
      {
        double sendTime;
        double eventTime;
        std::string packet;

/* 				std::cout << "Node " << GetNode()->GetId() << "-" << *i
                            << " " << m_peersDownloadSpeeds[*i] << " Mbps , time = "
                            << Simulator::Now ().GetSeconds() << "s \n"; */

        if(count < m_noMiners - 1)
        {
          int    noTransactions = static_cast<int>((m_nextBlockSize - m_blockHeadersSizeBytes)/m_averageTransactionSize);
          long   blockSize = m_blockHeadersSizeBytes + m_transactionIndexSize*noTransactions;
          sendTime = blockSize / m_uploadSpeed;

          m_nodeStats->blockSentBytes += m_bitcoinMessageHeader + blockSize;

          if (m_sendCompressedBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendCompressedBlockTimes.back())
          {
            eventTime = 0;
          }
          else
          {
            //std::cout << "m_sendCompressedBlockTimes.back() = m_sendCompressedBlockTimes.back() = " << m_sendCompressedBlockTimes.back() << std::endl;
            eventTime = m_sendCompressedBlockTimes.back() - Simulator::Now ().GetSeconds();
          }
          m_sendCompressedBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);

          //std::cout << sendTime << " " << eventTime << " " << m_sendCompressedBlockTimes.size() << std::endl;
          NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << *i
                      << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");

          //sendTime = blockSize / m_uploadSpeed * count;
          //std::cout << sendTime << std::endl;

          std::string packet = blockInfo.GetString();
          Simulator::Schedule (Seconds(sendTime), &BlockchainValidator::SendBlock, this, packet, m_peersSockets[*i]);
          Simulator::Schedule (Seconds(eventTime + sendTime), &BlockchainValidator::RemoveCompressedBlockSendTime, this);
        }
        else
        {
          sendTime = m_nextBlockSize / m_uploadSpeed;
          m_nodeStats->blockSentBytes += m_bitcoinMessageHeader + m_nextBlockSize;
          if (m_sendBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendBlockTimes.back())
          {
            eventTime = 0;
          }
          else
          {
            //std::cout << "m_sendBlockTimes.back() = m_sendBlockTimes.back() = " << m_sendBlockTimes.back() << std::endl;
            eventTime = m_sendBlockTimes.back() - Simulator::Now ().GetSeconds();
          }
          m_sendBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);
          packet = invInfo.GetString();

          /* std::cout << sendTime << " " << eventTime << " " << m_sendBlockTimes.size() << std::endl; */
          NS_LOG_INFO("Node " << GetNode()->GetId() << " will send the block to " << *i
                      << " at " << Simulator::Now ().GetSeconds() + eventTime << ", eventTime = " << eventTime  << "\n");

          Simulator::Schedule (Seconds(eventTime), &BlockchainValidator::SendBlock, this, packet, m_peersSockets[*i]);
          Simulator::Schedule (Seconds(eventTime + sendTime), &BlockchainValidator::RemoveSendTime, this);

        }
	   break;
      }
    }


/* 	//Send large packet
	int k;
	for (k = 0; k < 4; k++)
	{
      ns3TcpSocket->Send (reinterpret_cast<const uint8_t*>(packetInfo.GetString()), packetInfo.GetSize(), 0);
	  ns3TcpSocket->Send (delimiter, 1, 0);
	} */


  }

  m_minerAverageBlockGenInterval = m_minerGeneratedBlocks/static_cast<double>(m_minerGeneratedBlocks+1)*m_minerAverageBlockGenInterval
                                 + (Simulator::Now ().GetSeconds () - m_previousBlockGenerationTime)/(m_minerGeneratedBlocks+1);
  m_minerAverageBlockSize = m_minerGeneratedBlocks/static_cast<double>(m_minerGeneratedBlocks+1)*m_minerAverageBlockSize
                          + static_cast<double>(m_nextBlockSize)/(m_minerGeneratedBlocks+1);
  m_previousBlockGenerationTime = Simulator::Now ().GetSeconds ();
  m_minerGeneratedBlocks++;

  ScheduleNextMiningEvent ();
}

void
BlockchainValidator::ReceivedHigherBlock(const Block &newBlock)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_WARN("Blockchain validator " << GetNode ()->GetId () << " added a new block in the m_blockchain with higher height: " << newBlock);
  Simulator::Cancel (m_nextMiningEvent);
  ScheduleNextMiningEvent ();
}


void
BlockchainValidator::SendBlock(std::string packetInfo, Ptr<Socket> to)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("SendBlock: At time " << Simulator::Now ().GetSeconds ()
               << "s bitcoin miner " << GetNode ()->GetId () << " send "
               << packetInfo << " to " << to);

  rapidjson::Document d;

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  d.Parse(packetInfo.c_str());
  d.Accept(writer);

/*   if (d["type"] != "compressed-block")
    m_sendBlockTimes.erase(m_sendBlockTimes.begin()); */
  SendMessage(NO_MESSAGE, BLOCK, d, to);
  m_nodeStats->blockSentBytes -= m_bitcoinMessageHeader + d["blocks"][0]["size"].GetInt();
}
} // Namespace ns3


static double GetWallTime()
{
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
