/**
 * This file contains the definitions of the functions declared in blockchain-validator-helper.h
 */

#include "ns3/blockchain-validator-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/blockchain-validator.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

BlockchainValidatorHelper::BlockchainValidatorHelper (std::string protocol, Address address, std::vector<Ipv6Address> peers, int noMiners,
                                        std::map<Ipv6Address, double> &peersDownloadSpeeds, std::map<Ipv6Address, double> &peersUploadSpeeds,
                                        nodeInternetSpeeds &internetSpeeds, double hashRate) :
                                        BlockchainNodeHelper (),  m_minerType (NORMAL_MINER), m_blockBroadcastType (STANDARD),
                                        m_secureBlocks (6), m_blockGenBinSize (-1), m_blockGenParameter (-1)
{
  m_factory.SetTypeId ("ns3::BlockchainValidator");
  commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds);

  m_noMiners = noMiners;
  m_hashRate = hashRate;

  m_factory.Set ("NumberOfMiners", UintegerValue(m_noMiners));
  m_factory.Set ("HashRate", DoubleValue(m_hashRate));

}

Ptr<Application>
BlockchainValidatorHelper::InstallPriv (Ptr<Node> node) //FIX ME
{

   /*switch (m_minerType)
   {
      case NORMAL_MINER:
      {*/
        Ptr<BlockchainValidator> app = m_factory.Create<BlockchainValidator> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      /*}
      case SIMPLE_ATTACKER:
      {
        Ptr<BlockchainSimpleAttacker> app = m_factory.Create<BlockchainSimpleAttacker> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
      case SELFISH_MINER:
      {
        Ptr<BlockchainMaliciousNode> app = m_factory.Create<BlockchainMaliciousNode> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
      case SELFISH_MINER_TRIALS:
      {
        Ptr<BlockchainMaliciousNodeTrials> app = m_factory.Create<BlockchainMaliciousNodeTrials> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
   }*/

}

enum MinerType
BlockchainValidatorHelper::GetMinerType(void)
{
  return m_minerType;
}

void
BlockchainValidatorHelper::SetMinerType (enum MinerType m)  //FIX ME
{
   m_minerType = m;

   /*switch (m)
   {
      case NORMAL_MINER:
      {*/
        m_factory.SetTypeId ("ns3::BlockchainValidator");
        SetFactoryAttributes();
        /*break;
      }
      case SIMPLE_ATTACKER:
      {
        m_factory.SetTypeId ("ns3::BlockchainSimpleAttacker");
        SetFactoryAttributes();
        m_factory.Set ("SecureBlocks", UintegerValue(m_secureBlocks));

        break;
      }
      case SELFISH_MINER:
      {
        m_factory.SetTypeId ("ns3::BlockchainMaliciousNode");
        SetFactoryAttributes();

        break;
      }
      case SELFISH_MINER_TRIALS:
      {
        m_factory.SetTypeId ("ns3::BlockchainMaliciousNodeTrials");
        SetFactoryAttributes();
        m_factory.Set ("SecureBlocks", UintegerValue(m_secureBlocks));

        break;
      }
   }*/
}


void
BlockchainValidatorHelper::SetBlockBroadcastType (enum BlockBroadcastType m)
{
  m_blockBroadcastType = m;
}


void
BlockchainValidatorHelper::SetFactoryAttributes (void)
{
  m_factory.Set ("Protocol", StringValue (m_protocol));
  m_factory.Set ("Local", AddressValue (m_address));
  m_factory.Set ("NumberOfMiners", UintegerValue(m_noMiners));
  m_factory.Set ("HashRate", DoubleValue(m_hashRate));

  if (m_blockGenBinSize > 0 && m_blockGenParameter)
  {
    m_factory.Set ("BlockGenBinSize", DoubleValue(m_blockGenBinSize));
    m_factory.Set ("BlockGenParameter", DoubleValue(m_blockGenParameter));
  }
}

} // namespace ns3
