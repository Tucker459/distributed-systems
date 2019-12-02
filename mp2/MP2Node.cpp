/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 **********************************/
#include "MP2Node.h"
#include <stack>

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
}

/**
 * FUNCTION NAME: combine
 * 
 * DESCRIPTION: Combines two integers together
 */
int MP2Node::combine(int integer1, int integer2, int *operationTransID) {
	if(integer2 == 0) {
		integer2 = 1000;
	}

    int times = 1;
    while (times <= integer2) {
		times *= 10;
	}

	*operationTransID = integer2; 
    return integer1*times + integer2;
}

/**
 * FUNCTION NAME: splitInteger
 * 
 * DESCRIPTION: Split integer into separate digits
 */
stack<int> MP2Node::splitInteger(Message recvMsg) {
	stack<int> digits;
	int num = recvMsg.transID;
	while( num > 0)
	{
		digits.push(num % 10);
		num = num / 10;
	}

	return digits;
}

/**
 * FUNCTION NAME: sndCoordinatorMsg
 * 
 * DESCRIPTION: Sends the Coordinator's Failure or Success Message
 */
void MP2Node::sndCoordinatorMsg(Message recvMsg, int replyMsgType) {
	switch(replyMsgType) {
		case 1:
		  if(quorumInfo.cachedCreateTransID[recvMsg.transID] == 0) {
			  quorumInfo.quorumSuccessCnt[recvMsg.transID] = 0;
			  quorumInfo.quorumFailureCnt[recvMsg.transID] = 0;
			  quorumInfo.cachedCreateTransID[recvMsg.transID] = 1;
		  } else {
			  quorumInfo.cachedCreateTransID[recvMsg.transID]++;
		  }
		  break;

		case 2:
		  if(quorumInfo.cachedDeleteTransID[recvMsg.transID] == 0) {
			  quorumInfo.quorumSuccessCnt[recvMsg.transID] = 0;
			  quorumInfo.quorumFailureCnt[recvMsg.transID] = 0;
			  quorumInfo.cachedDeleteTransID[recvMsg.transID] = 1;
		  } else {
			  quorumInfo.cachedDeleteTransID[recvMsg.transID]++;
		  }
		  break;

		case 3:
          if(quorumInfo.cachedReadTransID[recvMsg.transID] == 0) {
			  quorumInfo.quorumSuccessCnt[recvMsg.transID] = 0;
			  quorumInfo.quorumFailureCnt[recvMsg.transID] = 0;
			  quorumInfo.cachedReadTransID[recvMsg.transID] = 1;
		  } else {
			  quorumInfo.cachedReadTransID[recvMsg.transID]++;
		  }
		  break;

		default:
		   if(quorumInfo.cachedUpdateTransID[recvMsg.transID] == 0) {
			   quorumInfo.quorumSuccessCnt[recvMsg.transID] = 0;
			   quorumInfo.quorumFailureCnt[recvMsg.transID] = 0;
			   quorumInfo.cachedUpdateTransID[recvMsg.transID] = 1;
		   } else {
			   quorumInfo.cachedUpdateTransID[recvMsg.transID]++;
		   }
	}

	if(replyMsgType == 3) {
		if(recvMsg.value != "") {
			quorumInfo.quorumSuccessCnt[recvMsg.transID]++;
			if(quorumInfo.quorumSuccessCnt[recvMsg.transID] == 2) {
				sndMsg(recvMsg,replyMsgType,true);
			}
		} else {
			quorumInfo.quorumFailureCnt[recvMsg.transID] = quorumInfo.quorumFailureCnt[recvMsg.transID] + 1;
			if(quorumInfo.quorumFailureCnt[recvMsg.transID] == 2) {
				sndMsg(recvMsg,replyMsgType,false);
			}
		}
	} else if (recvMsg.success) {
		quorumInfo.quorumSuccessCnt[recvMsg.transID]++;
		if(quorumInfo.quorumSuccessCnt[recvMsg.transID] == 2) {
			sndMsg(recvMsg,replyMsgType,true);
		}
	} else {
		quorumInfo.quorumFailureCnt[recvMsg.transID]++;
		if(quorumInfo.quorumFailureCnt[recvMsg.transID] == 2) {
			sndMsg(recvMsg,replyMsgType,false);
		}
	}

}

void MP2Node::sndMsg(Message recvMsg, int replyMsgType, bool isSuccessful) {
	switch(replyMsgType) {
		case 1:
		  if(isSuccessful) {
			  log->logCreateSuccess(&(memberNode->addr),true,recvMsg.transID,kvData[recvMsg.transID].front(),kvData[recvMsg.transID].back());
		  } else {
			  log->logCreateFail(&(memberNode->addr),true,recvMsg.transID,kvData[recvMsg.transID].front(),kvData[recvMsg.transID].back());
		  }
		  break;

		case 2:
		  if(isSuccessful) {
			  log->logDeleteSuccess(&(memberNode->addr),true,recvMsg.transID,kvData[recvMsg.transID].front());
		  } else {
			  log->logDeleteFail(&(memberNode->addr),true,recvMsg.transID,kvData[recvMsg.transID].front());
		  }
		  break;

		case 3:
		  if(isSuccessful) {
			  log->logReadSuccess(&(memberNode->addr),true,recvMsg.transID,kvData[recvMsg.transID].front(),kvData[recvMsg.transID].back());
			  qCheck[recvMsg.transID].sntSuccessMsg = true;
			  qCheck[recvMsg.transID].hasQuorum = true;
		  } else {
			  log->logReadFail(&(memberNode->addr),true,recvMsg.transID,kvData[recvMsg.transID].front());
			  qCheck[recvMsg.transID].sntFailureMsg = true;
			  qCheck[recvMsg.transID].hasQuorum = true;
		  }
		  break;

		default:
		  if(isSuccessful) {
			  log->logUpdateSuccess(&(memberNode->addr),true,recvMsg.transID,kvData[recvMsg.transID].front(),kvData[recvMsg.transID].back());
			  qCheck[recvMsg.transID].sntSuccessMsg = true;
			  qCheck[recvMsg.transID].hasQuorum = true;
		  } else {
			  log->logUpdateFail(&(memberNode->addr),true,recvMsg.transID,kvData[recvMsg.transID].front(),kvData[recvMsg.transID].back());
			  qCheck[recvMsg.transID].sntFailureMsg = true;
			  qCheck[recvMsg.transID].hasQuorum = true;
		  }
	}
}

void MP2Node::sndMsg(Address coordinatorAddr, int replyMsgType, int transID, string key, string value) {
	switch(replyMsgType) {
		case 3:
		  log->logReadFail(&(coordinatorAddr),true,transID,key);
		  break;

		default:
		  log->logUpdateFail(&(coordinatorAddr),true,transID,key,value);
	}
}

/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol (MP1Node)
 * 				   The membership list is returned as a vector of Nodes. See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> curMemList;
	bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode

	sort(curMemList.begin(), curMemList.end());
	if(ring.size() != curMemList.size()) {
		ring.clear();
		for(unsigned int i = 0; i < curMemList.size(); i++) {
			ring.emplace_back(curMemList.at(i));
		} 
		change = true;
	}

	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED & Check current unfinshed operations
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
	if(ht->currentSize() > 0 && change) {
		stabilizationProtocol();

		int ringCnt;
		for(int i = 31000; i <= g_transID + 40000; i++) {
			ringCnt = 0;
			if(!qCheck[i].hasQuorum && qCheck[i].lastUpdatedTime != 0) {
				for(std::vector<Node>::iterator it = ring.begin(); it != ring.end(); ++it) {
					if(it->getAddress()->getAddress() == qCheck[i].pReplica || it->getAddress()->getAddress() == qCheck[i].sReplica || it->getAddress()->getAddress() == qCheck[i].tReplica ) {
						ringCnt++;
						continue;
					}
				}

				if(ringCnt < 2 && qCheck[i].coolDwn != true) {
					qCheck[i].sntFailureMsg = true;
					qCheck[i].hasQuorum = true;
					if(qCheck[i].operation == READ) {
						sndMsg(qCheck[i].coordinatorAddr,3,qCheck[i].qTransID,qCheck[i].key,"");
						qCheck[i].lastUpdatedTime = par->getcurrtime();
					} else {
						sndMsg(qCheck[i].coordinatorAddr,4,qCheck[i].qTransID,qCheck[i].key,qCheck[i].value);
						qCheck[i].lastUpdatedTime = par->getcurrtime();
					}
					qCheck[i].coolDwn = true;
				}
			}
		}
	}
}

/**
 * FUNCTION NAME: getMembershipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) {
	/*
	 * Implement this
	 */
	MessageType msgType = CREATE;

	vector<Node> replicaNodes;
	replicaNodes = findNodes(key);
	int transID = 1;
	int combinedNum = combine(transID,g_transID, &g_transID);
	kvData[combinedNum].emplace_back(key);
	kvData[combinedNum].emplace_back(value);

	for(int i = 0; i < replicaNodes.size(); i++){
		string msg;
		ReplicaType repType;
		if(i == 0) {
			repType = PRIMARY;
		}else if(i == 1) {
			repType = SECONDARY;
		} else {
			repType = TERTIARY;
		}

		msg = Message(combinedNum,this->memberNode->addr, msgType, key, value, repType).toString();
		emulNet->ENsend(&(this->memberNode->addr), replicaNodes.at(i).getAddress(), msg);
	}
	g_transID++;
}

/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){
	/*
	 * Implement this
	 */

	MessageType msgType = READ;

	vector<Node> replicaNodes;
	replicaNodes = findNodes(key);
	int transID = 3;
	int combinedNum = combine(transID,g_transID, &g_transID);
	kvData[combinedNum].emplace_back(key);
	// Add to global operation checker
	qCheck[combinedNum].qTransID = combinedNum;
	qCheck[combinedNum].operation = msgType;
	qCheck[combinedNum].coordinatorAddr = memberNode->addr;
	qCheck[combinedNum].key = key;
	qCheck[combinedNum].lastUpdatedTime = par->getcurrtime();


	for(int i = 0; i < replicaNodes.size(); i++){
		string msg;
		msg = Message(combinedNum,this->memberNode->addr, msgType, key).toString();
		emulNet->ENsend(&(this->memberNode->addr), replicaNodes.at(i).getAddress(), msg);
		if(i == 0) {
			qCheck[combinedNum].pReplica = replicaNodes.at(i).getAddress()->getAddress();
		} else if(i == 1) {
			qCheck[combinedNum].sReplica = replicaNodes.at(i).getAddress()->getAddress();
		} else {
			qCheck[combinedNum].tReplica = replicaNodes.at(i).getAddress()->getAddress();
		}
	}
	g_transID++;
}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){
	/*
	 * Implement this
	 */

	MessageType msgType = UPDATE;

	vector<Node> replicaNodes;
	replicaNodes = findNodes(key);
	int transID = 4;
	int combinedNum = combine(transID,g_transID, &g_transID);
	kvData[combinedNum].emplace_back(key);
	kvData[combinedNum].emplace_back(value);
	// Add to global operation checker
	qCheck[combinedNum].qTransID = combinedNum;
	qCheck[combinedNum].operation = msgType;
	qCheck[combinedNum].coordinatorAddr = memberNode->addr;
	qCheck[combinedNum].key = key;
	qCheck[combinedNum].value = value;
	qCheck[combinedNum].lastUpdatedTime = par->getcurrtime();

	for(int i = 0; i < replicaNodes.size(); i++){
		string msg;
		ReplicaType repType;
		if(i == 0) {
			repType = PRIMARY;
			qCheck[combinedNum].pReplica = replicaNodes.at(i).getAddress()->getAddress();
		}else if(i == 1) {
			repType = SECONDARY;
			qCheck[combinedNum].sReplica = replicaNodes.at(i).getAddress()->getAddress();
		} else {
			repType = TERTIARY;
			qCheck[combinedNum].tReplica = replicaNodes.at(i).getAddress()->getAddress();
		}

		msg = Message(combinedNum,this->memberNode->addr, msgType, key, value, repType).toString();
		emulNet->ENsend(&(this->memberNode->addr), replicaNodes.at(i).getAddress(), msg);
	}
	g_transID++;
}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){
	/*
	 * Implement this
	 */

	MessageType msgType = DELETE;

	vector<Node> replicaNodes;
	replicaNodes = findNodes(key);
	int transID = 2;
	int combinedNum = combine(transID,g_transID, &g_transID);
	kvData[combinedNum].emplace_back(key);


	for(int i = 0; i < replicaNodes.size(); i++){
		string msg;

		msg = Message(combinedNum,this->memberNode->addr, msgType, key).toString();
		emulNet->ENsend(&(this->memberNode->addr), replicaNodes.at(i).getAddress(), msg);
	}
	g_transID++;
}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table
 * 			   	2) Return true or false based on success or failure
 */
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Insert key, value, replicaType into the hash table
	string entryValue;
	bool created;
	entryValue = Entry(value, par->getcurrtime(), replica).convertToString();
	created = ht->create(key, entryValue);

	if(quorumInfo.stabilMsg == false) {
		if(created) {
			log->logCreateSuccess(&(memberNode->addr), false, g_transID, key, value);
		} else {
			log->logCreateFail(&(memberNode->addr), false, g_transID, key, value);
		}
	}

	return created;
}

/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key) {
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value
	string val;
	val = ht->read(key);
	
	if(val != "") {
		log->logReadSuccess(&(memberNode->addr), false, g_transID, key, val);
	} else {
		log->logReadFail(&(memberNode->addr), false, g_transID, key);
	}

	return val;
}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false

	bool isUpdated;
	string entryVal;
	entryVal = Entry(value, par->getcurrtime(), replica).convertToString();
	isUpdated = ht->update(key,entryVal);

	if(isUpdated){
		log->logUpdateSuccess(&(memberNode->addr), false, g_transID, key, value);
	} else {
		log->logUpdateFail(&(memberNode->addr), false, g_transID, key, value);
	}

	return isUpdated;
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key) {
	/*
	 * Implement this
	 */
	// Delete the key from the local hash table
	bool deletedKey;
	deletedKey = ht->deleteKey(key);

	if(deletedKey) {
		log->logDeleteSuccess(&(memberNode->addr), false, g_transID, key);
	} else {
		log->logDeleteFail(&(memberNode->addr), false, g_transID, key);
	}

	return deletedKey;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 */
void MP2Node::checkMessages() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	char * data;
	int size;

	/*
	 * Declare your local variables here
	 */

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size); // C++ ; Calling  

		/*
		 * Handle the message types here
		 */
		
		Message recvMsg(message);

		if(recvMsg.type == CREATE) {
			bool createdKey = createKeyValue(recvMsg.key, recvMsg.value, recvMsg.replica);
			MessageType replyMsgType = REPLY;
			if(createdKey) {
				string repMsg = Message(recvMsg.transID,memberNode->addr,replyMsgType,true).toString();
				emulNet->ENsend(&(memberNode->addr),&(recvMsg.fromAddr),repMsg);
			} else {
				string repMsg = Message(recvMsg.transID,memberNode->addr,replyMsgType,false).toString();
				emulNet->ENsend(&(memberNode->addr),&(recvMsg.fromAddr),repMsg);
			}
			
		} else if(recvMsg.type == UPDATE) {
			bool isUpdated = updateKeyValue(recvMsg.key,recvMsg.value,recvMsg.replica);
			MessageType replyMsgType = REPLY;

			if(isUpdated) {
				string repMsg = Message(recvMsg.transID,memberNode->addr,replyMsgType,true).toString();
				emulNet->ENsend(&(memberNode->addr),&(recvMsg.fromAddr),repMsg);
			} else {
				string repMsg = Message(recvMsg.transID,memberNode->addr,replyMsgType,false).toString();
				emulNet->ENsend(&(memberNode->addr),&(recvMsg.fromAddr),repMsg);
			}

		} else if(recvMsg.type == READ) {
			string keyVal = readKey(recvMsg.key);
			kvData[recvMsg.transID].emplace_back(keyVal);
			qCheck[recvMsg.transID].value = keyVal;
			string repMsg = Message(recvMsg.transID,memberNode->addr,keyVal).toString();
			emulNet->ENsend(&(memberNode->addr),&(recvMsg.fromAddr),repMsg);

		} else if(recvMsg.type == DELETE) {
			bool deletedKey = deletekey(recvMsg.key);
			MessageType replyMsgType = REPLY;
			if(deletedKey) {
				string repMsg = Message(recvMsg.transID,memberNode->addr,replyMsgType,true).toString();
				emulNet->ENsend(&(memberNode->addr),&(recvMsg.fromAddr),repMsg);
			} else {
				string repMsg = Message(recvMsg.transID,memberNode->addr,replyMsgType,false).toString();
				emulNet->ENsend(&(memberNode->addr),&(recvMsg.fromAddr),repMsg);
			}

		} else if(recvMsg.type == READREPLY) {
			stack<int> digits = splitInteger(recvMsg);
			sndCoordinatorMsg(recvMsg,digits.top());
			
		} else {
			if(quorumInfo.stabilMsg == false) {
				stack<int> digits = splitInteger(recvMsg);
				sndCoordinatorMsg(recvMsg,digits.top());
			}
		}
	}

	/*
	 * This function should also ensure all READ and UPDATE operation
	 * get QUORUM replies
	 */
}

/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}
/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */
void MP2Node::stabilizationProtocol() {
	/*
	 * Implement this
	 */

	map<string, string>::iterator search;
	vector<Node> allReplicas;

	for(search = ht->hashTable.begin(); search != ht->hashTable.end(); search++) {
		quorumInfo.stabilMsg = true;
		clientCreate(search->first, search->second);
	} 
}
