#ifndef IOT_SENSOR_NODE_H
#define IOT_SENSOR_NODE_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/ipv6-address.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/blockchain.h"
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"
#include <random>
#include <cryptopp/rsa.h>
#include <cryptopp/aes.h>
#include <cryptopp/osrng.h>

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

								IotSensorNode(CryptoPP::RSA::PublicKey gatewayKey, CryptoPP::RSA::PrivateKey privKey);

								IotSensorNode(CryptoPP::RSA::PublicKey gatewayKey, CryptoPP::RSA::PrivateKey privKey,CryptoPP::RSA::PublicKey pubKey);

								IotSensorNode(CryptoPP::RSA::PublicKey gatewayKey);

								virtual ~IotSensorNode ();

								/**
								 * \return pointer to listening socket
								 */
								Ptr<Socket> GetListeningSocket (void) const;

								/**
								 * \return a vector containing the addresses of peers
								 */
								std::vector<Ipv6Address> GetPeersAddresses (void) const;

								/**
								 * \return the node's public key
								 */
								CryptoPP::RSA::PublicKey GetPublickey (void) const;

								/**
								 * \return the node's public key
								 */
								CryptoPP::RSA::PrivateKey GetPrivatekey (void) const;

								/**
								 * \return the node's gateway public key
								 */
								CryptoPP::RSA::PublicKey GetGatewayPublickey (void) const;

								/**
								 * \brief Revoke and change the node's keys
								 *	\param new private key and public key to be updated.
								 */
								 void SetNewKeys(CryptoPP::RSA::PrivateKey newPrivKey, CryptoPP::RSA::PublicKey newPublicKey);

								 /**
 								 * \brief Update the node's gateway public key
 								 *	\param new public key of the gateway
 								 */
								 void SetGatewayPublickey (CryptoPP::RSA::PublicKey newGatewayKey);

								/**
								 * \brief Set the addresses of peers
								 * \param peers the reference of a vector containing the Ipv6 addresses of peers
								 */
								void SetPeersAddresses (const std::vector<Ipv6Address> &peers);

								/**
								 * \brief adds a peer to the set of peers
								 * \param newPeer address of the peer to be added
								 **/
								void AddPeer (Ipv6Address newPeer);

								/**
								 * \brief set the download speeds of peers
								 * \param peersDownloadSpeeds the reference of a map containing the Ipv6 addresses of peers and their corresponding download speed
								 */
								void SetPeersDownloadSpeeds (const std::map<Ipv6Address, double> &peersDownloadSpeeds);

								/**
								 * \brief Set the upload speeds of peers
								 * \param peersUploadSpeeds the reference of a map containing the Ipv6 addresses of peers and their corresponding upload speed
								 */
								void SetPeersUploadSpeeds (const std::map<Ipv6Address, double> &peersUploadSpeeds);

								/**
								 * \brief Set the internet speeds of the node
								 * \param internetSpeeds a struct containing the download and upload speed of the node
								 */
								void SetNodeInternetSpeeds (const nodeInternetSpeeds &internetSpeeds);

								/**
								 * \brief Set the node statistics
								 * \param nodeStats a reference to a nodeStatistics struct
								 */
								void SetNodeStats (nodeStatistics *nodeStats);

								/**
								 * \brief Set the protocol type(default: STANDARD_PROTOCOL)
								 * \param protocolType the type of protocol used for advertising new blocks
								 */
								void SetProtocolType (enum ProtocolType protocolType);

								/**
								 * \brief Sends a message to a peer
								 * \param receivedMessage the type of the received message
								 * \param responseMessage the type of the response message
								 * \param d the rapidjson document containing the info of the outgoing message
								 * \param outgoingSocket the socket of the peer
								 */
								void SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, rapidjson::Document &d, Ptr<Socket> outgoingSocket);

								/**
								 * \brief Sends a message to a peer
								 * \param receivedMessage the type of the received message
								 * \param responseMessage the type of the response message
								 * \param d the rapidjson document containing the info of the outgoing message
								 * \param outgoingAddress the Address of the peer
								 */
								void SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, rapidjson::Document &d, Address &outgoingAddress);

								/**
								 * \brief Sends a message to a peer
								 * \param receivedMessage the type of the received message
								 * \param responseMessage the type of the response message
								 * \param packet a string containing the info of the outgoing message
								 * \param outgoingAddress the Address of the peer
								 */
								void SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, std::string packet, Address &outgoingAddress);

								/**
								 * \brief Sends a message to a peer
								 * \param receivedMessage the type of the received message
								 * \param responseMessage the type of the response message
								 * \param packet a string containing the info of the outgoing message
								 * \param outgoingIpv6Address the Ipv6Address of the peer
								 */
								void SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, std::string packet, Ipv6Address &outgoingIpv6Address);

								/**
								 * \brief Sends a message to a peer
								 * \param receivedMessage the type of the received message
								 * \param responseMessage the type of the response message
								 * \param packet a string containing the info of the outgoing message
								 * \param outgoingSocket the socket of the peer
								 */
								void SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, std::string packet, Ptr<Socket> outgoingSocket);
protected:
								// inherited from Application base class.
								virtual void StartApplication (void); // Called at time specified by Start
								virtual void StopApplication (void); // Called at time specified by Stop

								virtual void DoDispose (void);

								/**
								 * \brief Handle a packet received by the application
								 * \param socket the receiving socket
								 */
								void HandleRead (Ptr<Socket> socket);
								/**
								 * \brief Handle an incoming connection
								 * \param socket the incoming connection socket
								 * \param from the address the connection is from
								 */
								void HandleAccept (Ptr<Socket> socket, const Address& from);

								/**
								 * \brief Handle an connection close
								 * \param socket the connected socket
								 */
								void HandlePeerClose (Ptr<Socket> socket);

								/**
								 * \brief Handle an connection error
								 * \param socket the connected socket
								 */
								void HandlePeerError (Ptr<Socket> socket);

								/**
								 * \brief Check signature integrity of a message
								 * \param message message to check integrity
								 * \param sign signature of the message
								 **/
								bool checkSign (std::string message, std::string sign);

								/**
								 * \brief Check signature integrity of a message
								 * \param message message to check integrity
								 * \param sign signature of the message
								 * \param sender address of the message sender
								 **/
								bool checkSign (std::string message, std::string sign, Ipv6Address sender);

								/**
								 * \brief decrypt a encrypted message
								 * \param message message to be decrypted
								 **/
								std::string decrypt (std::string message);

								/**
								 * \brief encrypt a message to be sent
								 * \param message message to be encrypted
								 * \param PublicKey public key of the receiver
								 **/
								std::string encrypt (std::string message,CryptoPP::RSA::PublicKey publicKey);

								/**
								 * \brief encrypt a message to be sent
								 * \param message message to be encrypted
								 * \param receiver receiver's ip address
								 * \param type of encryption - aes or rsa
								 **/
								std::string encrypt (std::string message, Ipv6Address receiver,int type);

								/**
								 * \brief encrypt a message to be sent
								 * \param message message to be encrypted
								 * \param aes_key AES session key
								 * \param iv IV for the aes
								 **/
								std::string encrypt (std::string message, byte* aes_key, byte* iv);

private:

								// In the case of TCP, each socket accept returns a new socket, so the
								// listening socket is stored separately from the accepted sockets
								Ptr<Socket>     m_socket;                    //!< Listening socket
								Address m_local;                              //!< Local address to bind to
								TypeId m_tid;                                 //!< Protocol TypeId
								int m_numberOfPeers;                          //!< Number of node's peers
								int m_gatewayNodeId;             //!< Node Id for the gateway
								Ipv6Address m_gatewayAddress;  //!< Ipv6 address of the gateway
								double m_downloadSpeed;                       //!< The download speed of the node in Bytes/s
								double m_uploadSpeed;                         //!< The upload speed of the node in Bytes/s
								CryptoPP::RSA::PrivateKey m_privateKey;								//!< Private key for the sensor node

								CryptoPP::AutoSeededRandomPool m_prng;					//!< PRNG used to generate keys.

								CryptoPP::RSA::PublicKey m_publicKey;								//!< Public key for the sensor node
								CryptoPP::RSA::PublicKey m_gatewayPublicKey;							//!<	public key for the gateway

								std::vector<Ipv6Address>                            m_peersAddresses;           //!< The addresses of peers
								std::map<Ipv6Address, double>                       m_peersDownloadSpeeds;      //!< The peersDownloadSpeeds of channels
								std::map<Ipv6Address, double>                       m_peersUploadSpeeds;        //!< The peersUploadSpeeds of channels
								std::map<Ipv6Address, double>                       m_peersSessionKeys;        //!< The session keys established with different peers
								std::map<Ipv6Address, Ptr<Socket> >                  m_peersSockets;            //!< The sockets of peers
								std::map<Address, std::string>                      m_bufferedData;             //!< map holding the buffered data from previous handleRead events
								std::map<Ipv6Address, CryptoPP::RSA::PublicKey> m_publicKeys; //!< map holding the publicKeys for the peers
								std::map<Ipv6Address, byte[16]> m_cacheSessionKeys; //!< map holding the cached session keys for peers
								std::map<Ipv6Address, byte[16]> m_cacheSessionIVs; //!< map holding the cached session IVs for peers
								std::map<Ipv6Address, std::vector<std::string> > m_messages; //!< map holding all the messages from different peers
								nodeStatistics                                     *m_nodeStats;                //!< struct holding the node stats
								enum ProtocolType m_protocolType;                                               //!< protocol type

								const int m_commPort;               //!< 5555
								const int m_secondsPerMin;             //!< 60

								/// Traced Callback: received packets, source address.
								TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;


};



} // namespace ns3

#endif /* IOT_SENSOR_NODE_H */
