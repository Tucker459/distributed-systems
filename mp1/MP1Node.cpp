/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"
#include <time.h>
#include <tuple>
#include <algorithm>
#include <chrono>
#include <thread>
#include <unistd.h>

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: myAddress
 *
 * DESCRIPTION: Given id and port return the Address of the node
 */
Address MP1Node::myAddress(int id, short port) {
	string addrStr;
    addrStr = to_string(id) + ":" + to_string(port); 
    Address newAddr(addrStr);

    return newAddr;
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);
	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    memberNode->nodeIndex = id - 1;
    memberNode->sendMessages = false;
    memberNode->bufferFull = false;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until sending ready to send Gossip messages only
    if(memberNode->sendMessages) {
        sendGossipMsg();

        // Wait until you're in the group...
        if( !memberNode->inGroup ) {
            return;
        }

        // ...then jump in and share your responsibilites!
        nodeLoopOps();
    }

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt; // elt = (void *) elt
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
    
    Member* myMember;
    myMember = (Member *)env;

    MessageHdr* incomingMsg;
    incomingMsg = (MessageHdr *)data;

    if(incomingMsg->msgType == JOINREQ) {

        char myAddr[6];
        memcpy(myAddr,(char *)(incomingMsg+1),sizeof(myMember->addr.addr));

        int id = 0;
        short port;
        memcpy(&id,&myAddr[0],sizeof(int));
        memcpy(&port,&myAddr[4],sizeof(short));
        // Create Address for the debug log
        Address toAddr = myAddress(id,port);

        long heartbeat;
        memcpy(&heartbeat,(char *)(incomingMsg+1) + 1 + sizeof(memberNode->addr.addr),sizeof(long));

        // std::cout << "----------------------" << std::endl;
        // std::cout << "Received JOINREQ Message!" << std::endl;
        // std::cout << "message type: " << incomingMsg->msgType << std::endl;
        // std::cout << "id: " << id << std::endl;
        // std::cout << "port: " << port << std::endl;
        // std::cout << "heartbeat: " <<  heartbeat << std::endl;
        // std::cout << "address: " << toAddr.getAddress() << std::endl;
    
        time_t timestampSec;
        time(&timestampSec);
        if(memberNode->memberList.size() == 0) {
            myMember->heartbeat = myMember->heartbeat + 1;
            int nodeOneID = 0;
            short nodeOnePort;
            memcpy(&nodeOneID,&myMember->addr.addr[0],sizeof(int));
            memcpy(&nodeOnePort,&myMember->addr.addr[4],sizeof(short));
            MemberListEntry memListEntry(nodeOneID,nodeOnePort,myMember->heartbeat,par->getcurrtime());
            memberNode->memberList.push_back(memListEntry);
        }
        MemberListEntry memListEntry(id,port,heartbeat,par->getcurrtime());
        memberNode->memberList.push_back(memListEntry);
        #ifdef DEBUGLOG
            log->logNodeAdd(&memberNode->addr, &toAddr);
        #endif
        
        size_t szOfList = memberNode->memberList.size();
        if(szOfList == 10) {
            std::cout << "Sending out to other nodes" << std::endl;
            size_t outgoingMsgSz = sizeof(MessageHdr) + sizeof(memberNode->addr.addr) + sizeof(szOfList) + (sizeof(MemberListInfo) * szOfList);
            MessageHdr* outgoingMsg;
            outgoingMsg = (MessageHdr *) malloc(outgoingMsgSz * sizeof(char));

            outgoingMsg->msgType = JOINREP;
            memcpy((char *)(outgoingMsg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
            memcpy((char *)(outgoingMsg+1) + sizeof(memberNode->addr.addr), &szOfList, sizeof(szOfList));
            int i = 0;
            MemberListInfo memInfo[szOfList];
            for(std::vector<MemberListEntry>::iterator cPos = memberNode->memberList.begin(); cPos != memberNode->memberList.end(); cPos++){
                memInfo[i].id = cPos->id;
                memInfo[i].port = cPos->port;
                memInfo[i].heartbeat = cPos->heartbeat;
                memInfo[i].timestamp = cPos->timestamp;
                memInfo[i].failure = false;
                memInfo[i].cleanup = false;
                i++;
            }
            // Putting the membership list into the outgoing msg
            memcpy((char *)(outgoingMsg+1) + sizeof(memberNode->addr.addr) + sizeof(szOfList), memInfo, (sizeof(MemberListInfo) * szOfList));

            for(std::vector<MemberListEntry>::iterator cPos = memberNode->memberList.begin(); cPos != memberNode->memberList.end(); cPos++){
                if(cPos->id == 1) {
                    continue;
                }
                Address tempAddr = myAddress(cPos->id, cPos->port);
    
                //send JOINREP message back to the member that requested to join the group
               emulNet->ENsend(&memberNode->addr, &tempAddr, (char *)outgoingMsg, outgoingMsgSz);
            }

            free(outgoingMsg);
        }

        return true;

    } else if(incomingMsg->msgType == JOINREP) {

        char myAddr[6];
        memcpy(myAddr,(char *)(incomingMsg+1),sizeof(myMember->addr.addr));

        size_t szOfList;
        memcpy(&szOfList,(char *)(incomingMsg+1) + sizeof(memberNode->addr.addr),sizeof(szOfList));

        //int myID[szOfList];
        MemberListInfo memInfo[szOfList];
        memcpy(&memInfo,(char *)(incomingMsg+1) + sizeof(memberNode->addr.addr) + sizeof(szOfList), (sizeof(MemberListInfo) * szOfList));
    
        std::cout << "----------------------" << std::endl;
        std::cout << "Received JOINREP Message!" << std::endl;
        std::cout << "message type: " << incomingMsg->msgType << std::endl;
        std::cout << "address: " << myMember->addr.getAddress() << std::endl;
        std::cout << "size of list: " << szOfList << std::endl;

    
        for(int i = 0; i != szOfList; i++){
           MemberListEntry memListEntry(memInfo[i].id,memInfo[i].port,memInfo[i].heartbeat,par->getcurrtime());
           memberNode->memberList.push_back(memListEntry);
           Address tmpAddr = myAddress(memInfo[i].id,memInfo[i].port);
           #ifdef DEBUGLOG
                log->logNodeAdd(&memberNode->addr, &tmpAddr);
           #endif
        }

        memberNode->inGroup = true;

        int i = 0;
        for(vector<MemberListEntry>::iterator myPos = memberNode->memberList.begin() ; myPos != memberNode->memberList.end() ; myPos++) {
            Address tempAddr = myAddress(myPos->id, myPos->port);
            if(0 == std::strcmp(tempAddr.addr, myMember->addr.addr)) {
                myPos->heartbeat = myPos->heartbeat + 1;
                myPos->timestamp = par->getcurrtime();
            }

            tempAddr = myAddress(memInfo[i].id, memInfo[i].port);
            if(0 == std::strcmp(tempAddr.addr, myMember->addr.addr)) {
                memInfo[i].heartbeat = memInfo[i].heartbeat + 1;
                memInfo[i].timestamp =  par->getcurrtime();
                break;
            }
            i++;
        }

        std::cout << "---Printing out membership list---" << std::endl;
        for(std::vector<MemberListEntry>::iterator myPos = memberNode->memberList.begin(); myPos != memberNode->memberList.end(); myPos++) {
            std::cout << "id: " << myPos->id << " port: " << myPos->port << " heartbeat: " << myPos->heartbeat << " timestamp: " << myPos->timestamp << std::endl;
        }

        if(szOfList > 1) {
            int indxNum = rand() % (szOfList - 1);
            MemberListInfo chosenNode = memInfo[indxNum];
            Address tempAddr = myAddress(chosenNode.id, chosenNode.port);
            while(0 == std::strcmp(tempAddr.addr, myMember->addr.addr)) {
                 indxNum = rand() % (szOfList - 1);
                 chosenNode = memInfo[indxNum];
                 tempAddr = myAddress(chosenNode.id, chosenNode.port);
            }
 
            size_t outgoingMsgSz = sizeof(MessageHdr) + sizeof(szOfList) + (sizeof(MemberListInfo) * szOfList);
            MessageHdr* outgoingMsg;
            outgoingMsg = (MessageHdr *) malloc(outgoingMsgSz * sizeof(char));

            outgoingMsg->msgType = GOSSIP;
            memcpy((char *)(outgoingMsg+1), &szOfList,sizeof(szOfList));
            memcpy((char *)(outgoingMsg+1) + sizeof(szOfList), &memInfo,sizeof(MemberListInfo) * szOfList);

            Address toAddr = myAddress(chosenNode.id,chosenNode.port); 
            // Sending my GOSSIP message to a random node in my membership list
            emulNet->ENsend(&memberNode->addr, &toAddr, (char *)outgoingMsg, outgoingMsgSz);

            free(outgoingMsg);
        }

        return true;
    
    } else if(incomingMsg->msgType == GOSSIP) {

        // std::cout << "----------------------" << std::endl;
        // std::cout << "Received GOSSIP Message!" << std::endl;
        memberNode->sendMessages = true;

        if(par->getcurrtime() % 10 == 0) {
            memberNode->bufferFull = false;
        }

        size_t szOfList;
        memcpy(&szOfList,(char *)(incomingMsg+1),sizeof(szOfList));

        MemberListInfo memInfo[szOfList];
        memcpy(&memInfo,(char *)(incomingMsg+1) + sizeof(szOfList), (sizeof(MemberListInfo) * szOfList));
        
        // for(int i = 0; i != szOfList; i++){
        //     std::cout << "id: " << memInfo[i].id << " port: " << memInfo[i].port << " heartbeat: " << memInfo[i].heartbeat << " timestamp: " << memInfo[i].timestamp << " failure: " << memInfo[i].failure  <<  " cleanup: " << memInfo[i].cleanup << std::endl;
        // }

        // Goes through the membership list of the node and check if any node needs to be updated based on the heartbeat counter of
        // incoming membership list that was sent.
        // If no node matches the incoming node's membership list it will be pushed back on the vector. 
        bool memberListUpdated;
        for(int i = 0; i != szOfList; i++) {
            for(std::vector<MemberListEntry>::iterator memPos = memberNode->memberList.begin(); memPos != memberNode->memberList.end(); memPos++) {
                if(memInfo[i].id == memPos->id) {
                    if(memInfo[i].heartbeat > memPos->heartbeat) {
                        memPos->heartbeat = memInfo[i].heartbeat;
                        memPos->timestamp = par->getcurrtime();
                        memberListUpdated = true;
                    }
                    break;
                }
            }
        }
    }
    return true;
}

/**
 * FUNCTION NAME: sendGossipMsg
 *
 * DESCRIPTION: Send Gossip Messages Once per round
 */
void MP1Node::sendGossipMsg() {

    // Only Send Gossip Messages if the buffer size is not full 
    if(!memberNode->bufferFull) {

        // Increase heartbeat
        for(vector<MemberListEntry>::iterator myPos = memberNode->memberList.begin() ; myPos != memberNode->memberList.end() ; myPos++) {
            Address tempAddr = myAddress(myPos->id, myPos->port);
            if(0 == std::strcmp(tempAddr.addr, memberNode->addr.addr)) {
                myPos->heartbeat = myPos->heartbeat + 1;
                myPos->timestamp = par->getcurrtime();
            }
        }

        std::cout << "---Printing out membership list---" << std::endl;
        std::cout << "Node: " << memberNode->addr.getAddress() << std::endl;
        for(std::vector<MemberListEntry>::iterator myPos = memberNode->memberList.begin(); myPos != memberNode->memberList.end(); myPos++) {
            std::cout << "id: " << myPos->id << " port: " << myPos->port << " heartbeat: " << myPos->heartbeat << " timestamp: " << myPos->timestamp << std::endl;
        }

        MemberListInfo memInfoUpdated[memberNode->memberList.size()];
        int i = 0;
        for(std::vector<MemberListEntry>::iterator myPos = memberNode->memberList.begin(); myPos != memberNode->memberList.end(); myPos++) {
            memInfoUpdated[i].id = myPos->id;
            memInfoUpdated[i].port = myPos->port;
            memInfoUpdated[i].heartbeat = myPos->heartbeat;
            memInfoUpdated[i].timestamp = myPos->timestamp;
            memInfoUpdated[i].failure = false;
            memInfoUpdated[i].cleanup = false;
            i++;
        }

    
        //random_shuffle(memberNode->memberList.begin(), memberNode->memberList.end());
        int firstIndx = memberNode->nodeIndex + 1; 
        int secondIndx = memberNode->nodeIndex + 2; 
        memberNode->nodeIndex = secondIndx;
        if(firstIndx > 9) {
            firstIndx = 0;
            secondIndx = 1;
            memberNode->nodeIndex = 1;
        } else if(secondIndx > 9) {
            secondIndx = 0;
            memberNode->nodeIndex = 0;
        }

        Address firstAddr = myAddress(memberNode->memberList[firstIndx].id, memberNode->memberList[firstIndx].port);
        Address secondAddr = myAddress(memberNode->memberList[secondIndx].id, memberNode->memberList[secondIndx].port);
        // Address thirdAddr = myAddress(memberNode->memberList[2].id, memberNode->memberList[2].port);
        // Address fourthAddr = myAddress(memberNode->memberList[3].id, memberNode->memberList[3].port);
        // while(0 == std::strcmp(firstAddr.addr, myMember->addr.addr) || 0 == std::strcmp(secondAddr.addr, myMember->addr.addr)) {
        //     random_shuffle(memberNode->memberList.begin(), memberNode->memberList.end());
        //     firstAddr = myAddress(memberNode->memberList[0].id, memberNode->memberList[0].port);
        //     secondAddr = myAddress(memberNode->memberList[1].id, memberNode->memberList[1].port);
        //     // thirdAddr = myAddress(memberNode->memberList[2].id, memberNode->memberList[2].port);
        //     // fourthAddr = myAddress(memberNode->memberList[3].id, memberNode->memberList[3].port);
        // }

        size_t szOfMemList = memberNode->memberList.size();
        size_t outgoingMsgSz = sizeof(MessageHdr) + sizeof(szOfMemList) + (sizeof(MemberListInfo) * szOfMemList);
        MessageHdr* outgoingMsg;
        outgoingMsg = (MessageHdr *) malloc(outgoingMsgSz * sizeof(char));

        outgoingMsg->msgType = GOSSIP;
        memcpy((char *)(outgoingMsg+1), &szOfMemList,sizeof(szOfMemList));
        memcpy((char *)(outgoingMsg+1) + sizeof(szOfMemList), &memInfoUpdated,sizeof(MemberListInfo) * szOfMemList);

        std::sort(memberNode->memberList.begin(), memberNode->memberList.end(), MemberListCompareByID());
        // Sending my GOSSIP messages to a random nodes in my membership list
        emulNet->ENsend(&memberNode->addr, &firstAddr, (char *)outgoingMsg, outgoingMsgSz);
        emulNet->ENsend(&memberNode->addr, &secondAddr, (char *)outgoingMsg, outgoingMsgSz);
        // emulNet->ENsend(&memberNode->addr, &thirdAddr, (char *)outgoingMsg, outgoingMsgSz);
        // emulNet->ENsend(&memberNode->addr, &fourthAddr, (char *)outgoingMsg, outgoingMsgSz);
        free(outgoingMsg);
    }
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */

    std::vector<MemberListEntry>::iterator myPos = memberNode->memberList.begin();

    while(myPos != memberNode->memberList.end()) {
        long msgDelay = par->getcurrtime() - myPos->timestamp;
        if(msgDelay >= 20) {
            Address removedAddr = myAddress(myPos->id, myPos->port);
            #ifdef DEBUGLOG
            log->logNodeRemove(&memberNode->addr, &removedAddr);
            #endif
            myPos = memberNode->memberList.erase(myPos);

        } else {
            myPos++;
        }

    }


    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
