#ifndef BLOCKCHAIN_MALICIOUS_NODE_TRIALS_H
#define BLOCKCHAIN_MALICIOUS_NODE_TRIALS_H

#include "blockchain-validator.h"


namespace ns3 {

class Address;
class Socket;
class Packet;

/**
 * \ingroup packetsink
 *
 * \brief Receive and consume traffic generated to an IP address and port
 *
 * This application was written to complement OnOffApplication, but it
 * is more general so a PacketSink name was selected.  Functionally it is
 * important to use in multicast situations, so that reception of the layer-2
 * multicast frames of interest are enabled, but it is also useful for
 * unicast as an example of how you can write something simple to receive
 * packets at the application layer.  Also, if an IP stack generates
 * ICMP Port Unreachable errors, receiving applications will be needed.
 *
 * The constructor specifies the Address (IP address and port) and the
 * transport protocol to use.   A virtual Receive () method is installed
 * as a callback on the receiving socket.  By default, when logging is
 * enabled, it prints out the size of packets and their address.
 * A tracing source to Receive() is also available.
 */
class BlockchainMaliciousNodeTrials : public BlockchainValidator
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  BlockchainMaliciousNodeTrials ();

  virtual ~BlockchainMaliciousNodeTrials (void);

protected:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  virtual void DoDispose (void);

  virtual void MineBlock (void);

  virtual void ReceivedHigherBlock(const Block &newBlock);	//Called for blocks with better score(height). Remove m_nextMiningEvent and call MineBlock again.

  uint32_t   m_secureBlocks;
  bool       m_attackFinished;
  int        m_winningStreak;
  int        m_trials;
  uint32_t   m_advertiseBlocks;

};

} // namespace ns3

#endif /* BLOCKCHAIN_MALICIOUS_NODE_TRIALS_H */
