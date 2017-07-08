/**
 * This file contains all the necessary enumerations and structs used throughout the simulation for the blockchain nodes.
 * It also defines 3 very important class; the Block, Chunk and Blockchain.
 */


#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <map>
#include "ns3/address.h"
#include <algorithm>

namespace ns3 {

/**
 * The different blockchain message and node message types that have been implemented.
 */
enum Messages
{
        INV,        //0
        GET_HEADERS, //1
        HEADERS,    //2
        GET_BLOCKS, //3
        BLOCK,      //4
        GET_DATA,   //5
        NO_MESSAGE, //6
        EXT_INV,    //7
        EXT_GET_HEADERS, //8
        EXT_HEADERS, //9
        EXT_GET_BLOCKS, //10
        EXT_GET_DATA, //12
        SEND_PUBLIC_KEY, //13
        RECEIVE_PUBLIC_KEY, //14
        RECEIVE_MESSAGE,  //15
        CREATE_ENTRY,  //16
        UPDATE_ENTRY,  //17
        DELETE_ENTRY,  //18
};


/**
 * The blockchain miner types that have been implemented. The first one is the normal miner (default), the second one is the gateway miner to keep 2 different types of blockchains, the last 3 are used to simulate different attacks.
 */
enum MinerType
{
        NORMAL_MINER,          //Default Miner  to validate new gateways
        GATEWAY_MINER,        //Maintains 2 different blockchains
        SIMPLE_ATTACKER,
        MALICIOUS_NODE,
        MALICIOUS_NODE_TRIALS
};


/**
 * The different block broadcast types that the node uses to adventize newly formed blocks.
 */
enum BlockBroadcastType
{
        STANDARD,              //DEFAULT
        UNSOLICITED,
        RELAY_NETWORK,
        UNSOLICITED_RELAY_NETWORK
};


/**
 * The protocol that the nodes use to advertise new blocks. The STANDARD_PROTOCOL (default) uses the standard INV messages for advertising,
 * whereas the SENDHEADERS uses HEADERS messages to advertise new blocks.
 */
enum ProtocolType
{
        STANDARD_PROTOCOL,     //DEFAULT
        SENDHEADERS
};


/**
 * The different IoT Manufacturers used for the simulation.
 */
enum ManufacturerID
{
        SAMSUNG, //0
        QUALCOMM,     //1
        CISCO, //2
        ERICSSON, //3
        IBM,  //4
        SILICON,     //5
        OTHER       //6
};


/**
 * The struct used for collecting node statistics.
 */
typedef struct {
        int nodeId;
        double meanBlockReceiveTime;
        double meanBlockPropagationTime;
        double meanBlockSize;
        int totalBlocks;
        int staleBlocks;
        int miner;                         //0->gateway, 1->miner
        int minerGeneratedBlocks;
        double minerAverageBlockGenInterval;
        double minerAverageBlockSize;
        double hashRate;
        int attackSuccess;                   //0->fail, 1->success
        long invReceivedBytes;
        long invSentBytes;
        long getHeadersReceivedBytes;
        long getHeadersSentBytes;
        long headersReceivedBytes;
        long headersSentBytes;
        long getDataReceivedBytes;
        long getDataSentBytes;
        long blockReceivedBytes;
        long blockSentBytes;
        long extInvReceivedBytes;
        long extInvSentBytes;
        long extGetHeadersReceivedBytes;
        long extGetHeadersSentBytes;
        long extHeadersReceivedBytes;
        long extHeadersSentBytes;
        long extGetDataReceivedBytes;
        long extGetDataSentBytes;
        long chunkReceivedBytes;
        long chunkSentBytes;
        int longestFork;
        int blocksInForks;
        int connections;
        long blockTimeouts;
        long chunkTimeouts;
        int minedBlocksInMainChain;
} nodeStatistics;


typedef struct {
        double downloadSpeed;
        double uploadSpeed;
} nodeInternetSpeeds;

typedef struct {
    int nodeId;
    int manufacturerId;
    Ipv6Address ipv6Address;
    std::string nodePublicKey;
    std::string signature;
} blockDataTuple;

/**
 * Fuctions used to convert enumeration values to the corresponding strings.
 */
const char* getMessageName(enum Messages m);
const char* getMinerType(enum MinerType m);
const char* getBlockBroadcastType(enum BlockBroadcastType m);
const char* getProtocolType(enum ProtocolType m);
const char* getManufacturerID(enum ManufacturerID m);
enum ManufacturerID getManufacturerEnum(uint32_t n);

class Block
{
public:
        // static blockDataTuple emptyBlockData = {0, 0, Ipv6Address("0::0::0::0"), "" ,""};
        Block (int blockHeight, int minerId, int parentBlockMinerId = 0, int blockSizeBytes = 0,
               double timeCreated = 0, std::map<int, blockDataTuple> blockDataMap = std::map<int, blockDataTuple>(), double timeReceived = 0, Ipv6Address receivedFromIpv6Address = Ipv6Address("0::0::0::0"));
        Block ();
        Block (const Block &blockSource); // Copy constructor
        virtual ~Block (void);

        int GetBlockHeight (void) const;
        void SetBlockHeight (int blockHeight);

        int GetMinerId (void) const;
        void SetMinerId (int minerId);

        int GetParentBlockMinerId (void) const;
        void SetParentBlockMinerId (int parentBlockMinerId);

        int GetBlockSizeBytes (void) const;
        void SetBlockSizeBytes (int blockSizeBytes);

        double GetTimeCreated (void) const;
        double GetTimeReceived (void) const;

        Ipv6Address GetReceivedFromIpv6Address (void) const;
        void SetReceivedFromIpv6Address (Ipv6Address receivedFromIpv6Address);


        blockDataTuple GetNodeData (int nodeId);
        void SetNodeData (int nodeId, blockDataTuple blockData);

        /**
         * Checks if the block provided as the argument is the parent of this block object
         */
        bool IsParent (const Block &block) const;

        /**
         * Checks if the block provided as the argument is a child of this block object
         */
        bool IsChild (const Block &block) const;

        Block& operator= (const Block &blockSource); //Assignment Constructor

        friend bool operator== (const Block &block1, const Block &block2);
        friend std::ostream& operator<< (std::ostream &out, const Block &block);

protected:
        int m_blockHeight;                    // The height of the block
        int m_minerId;                        // The id of the miner which mined this block
        int m_parentBlockMinerId;             // The id of the miner which mined the parent of this block
        int m_blockSizeBytes;                 // The size of the block in bytes
        double m_timeCreated;                 // The time the block was created
        double m_timeReceived;                // The time the block was received from the node
        Ipv6Address m_receivedFromIpv6Address;       // The Ipv6Address of the node which sent the block to the receiving node
        std::map <int, blockDataTuple> m_blockDataMap;           // Map containing all the data for a node.
};


class Blockchain
{
public:
        Blockchain(void);
        virtual ~Blockchain (void);

        int GetNoStaleBlocks (void) const;

        int GetNoOrphans (void) const;

        int GetTotalBlocks (void) const;

        int GetBlockchainHeight (void) const;

        /**
         * Check if the block has been included in the blockchain.
         */
        bool HasBlock (const Block &newBlock) const;
        bool HasBlock (int height, int minerId) const;

        /**
         * Get the nodes' public key ,its' signature and validate its' signature
         **/
        // std::string GetPublickey (int nodeId);
        // Block GetPublickeyBlock (int nodeId);
        // std::string GetNodePublicKeySignature (int nodeId);
        // bool CheckPublicKeySignature (int nodeId);

        /**
         * Return the block with the specified height and minerId.
         * Should be called after HasBlock() to make sure that the block exists.
         * Returns the orphan blocks too.
         */
        Block ReturnBlock(int height, int minerId);

        /**
         * Check if the block is an orphan.
         */
        bool IsOrphan (const Block &newBlock) const;
        bool IsOrphan (int height, int minerId) const;

        /**
         * Gets a pointer to the block.
         */
        const Block* GetBlockPointer (const Block &newBlock) const;

        /**
         * Gets the children of a block that are not orphans.
         */
        const std::vector<const Block *> GetChildrenPointers (const Block &block);

        /**
         * Gets the children of a newBlock that used to be orphans before receiving the newBlock.
         */
        const std::vector<const Block *> GetOrphanChildrenPointers (const Block &newBlock);

        /**
         * Gets the parent of a block
         */
        const Block* GetParent (const Block &block); //Get the parent of newBlock

        /**
         * Gets the current top block. If there are two block with the same height (siblings), returns the one received first.
         */
        const Block* GetCurrentTopBlock (void) const;

        /**
         * Adds a new block in the blockchain.
         */
        void AddBlock (const Block& newBlock);

        /**
         * Adds a new orphan block in the blockchain.
         */
        void AddOrphan (const Block& newBlock);

        /**
         * Removes a new orphan block in the blockchain.
         */
        void RemoveOrphan (const Block& newBlock);

        /**
         * Prints all the currently orphan blocks.
         */
        void PrintOrphans (void);

        /**
         * Gets the total number of blocks in forks.
         */
        int GetBlocksInForks (void);

        /**
         * Gets the longest fork size
         */
        int GetLongestForkSize (void);

        // void changePublicKey (Block& Block, std::string newPublicKey);
        // void changePublicKey (int nodeId, std::string newPublicKey);

        friend std::ostream& operator<< (std::ostream &out, Blockchain &blockchain);

private:
        int m_noStaleBlocks;                              //total number of stale blocks
        int m_totalBlocks;                                //total number of blocks including the genesis block
        std::vector<std::vector<Block> >    m_blocks;     //2d vector containing all the blocks of the blockchain. (row->blockHeight, col->sibling blocks)
        std::vector<Block>                 m_orphans;     //vector containing the orphans
        // std::map< int, Block > m_block_map;          //map containing the nodeId to block mapping
        // std::map<int, std::string> m_public_key_map;    //direct map containing mapping of node to public key


};


} // Namespace ns3

#endif /* BLOCKCHAIN_H */
