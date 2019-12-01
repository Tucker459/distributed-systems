/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"
#include <stack>

// Global TransID Variables 
static int createTransID = 0;
static int readTransID = 0;
static int updateTransID = 0;
static int deleteTransID = 0;

/**
 * Struct Name: quorum
 */
typedef struct quorum_attr {
	// Quorum Sucess Count
	map<int, int> quorumSuccessCnt;
	// Quorum Failure Count
	map<int, int> quorumFailureCnt; 
	// readQuorum Success Count
	map<int, int> readQuorumSuccessCnt;
	// readQuorum Failure Count
	map<int, int> readQuorumFailureCnt;
	// cachedCreateTransID
	map<int, int> cachedCreateTransID;
	// cachedReadTransID
	map<int, int> cachedReadTransID;
	// cachedUpdateTransID
	map<int, int> cachedUpdateTransID;
	// cachedDeleteTransID
	map<int, int> cachedDeleteTransID;
	// holds all of the keys and values based on transID
	map<int, std::vector<string>> kvData;
	// stabilization message 
	bool stabilMsg;
}quorum_attr;

/**
 * CLASS NAME: MP2Node
 *
 * DESCRIPTION: This class encapsulates all the key-value store functionality
 * 				including:
 * 				1) Ring
 * 				2) Stabilization Protocol
 * 				3) Server side CRUD APIs
 * 				4) Client side CRUD APIs
 */
class MP2Node {
private:
	// Vector holding the next two neighbors in the ring who have my replicas
	vector<Node> hasMyReplicas;
	// Vector holding the previous two neighbors in the ring whose replicas I have
	vector<Node> haveReplicasOf;
	// Ring
	vector<Node> ring;
	// Hash Table
	HashTable * ht;
	// Member representing this member
	Member *memberNode;
	// Params object
	Params *par;
	// Object of EmulNet
	EmulNet * emulNet;
	// Object of Log
	Log * log;
	// Holds Information related to reaching Quorum
	quorum_attr quorumInfo;
	// temp 
	int tmpCnt;


public:
	MP2Node(Member *memberNode, Params *par, EmulNet *emulNet, Log *log, Address *addressOfMember);
	Member * getMemberNode() {
		return this->memberNode;
	}

	// TransIDs Combine Function
	int combine(int integer1, int integer2);

	// Split Integer
	stack<int> splitInteger(Message recvMsg);

	// Send Coordinator Success or Failure Msg 
	void sndCoordinatorMsg(Message recvMsg, int replyMsgType);

	// Send Coordinator Message based on Message Type
	void sndMsg(Message recvMsg, int replyMsgType, bool isSuccessful);

	// ring functionalities
	void updateRing();
	vector<Node> getMembershipList();
	size_t hashFunction(string key);
	void findNeighbors();

	// client side CRUD APIs
	void clientCreate(string key, string value);
	void clientRead(string key);
	void clientUpdate(string key, string value);
	void clientDelete(string key);

	// receive messages from Emulnet
	bool recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);

	// handle messages from receiving queue
	void checkMessages();

	// coordinator dispatches messages to corresponding nodes
	void dispatchMessages(Message message);

	// find the addresses of nodes that are responsible for a key
	vector<Node> findNodes(string key);

	// server
	bool createKeyValue(string key, string value, ReplicaType replica);
	string readKey(string key);
	bool updateKeyValue(string key, string value, ReplicaType replica);
	bool deletekey(string key);

	// stabilization protocol - handle multiple failures
	void stabilizationProtocol();

	~MP2Node();
};

#endif /* MP2NODE_H_ */
