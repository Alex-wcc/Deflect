/*
 * q-routing.h
 *
 *  Created on: 2018-3-16
 *      Author: eric
 */

#ifndef SRC_NANONETWORKS_MODEL_Q_ROUTING_H_
#define SRC_NANONETWORKS_MODEL_Q_ROUTING_H_

#include "nano-routing-entity.h"
#include <list>
#include <iostream>
#include <utility>
#include <string>

namespace ns3 {

/*this class implements the q routing protocol.
 * Eric Wang*/

class QRouting: public NanoRoutingEntity {
public:
	static TypeId GetTypeId();

	QRouting();
	virtual ~QRouting();

	virtual void DoDispose();

public:
	void AddRoute(uint32_t dstId, uint32_t nextId, double Qvalue, uint32_t HopCount);
	uint32_t LookupRoute(uint32_t dstId);
	double SearchRouteForQvalue(uint32_t dstId);
	uint32_t SearchRouteForQHopCount(uint32_t dstId);
	bool RouteAvailable(uint32_t dstId);
	uint32_t LookUpPreNode(uint32_t dstId);
	bool SearchPreNode(uint32_t dstId, uint32_t nextId);//寻找对应的记录是否存在，以便于进行添加或者更新
	uint32_t ChooseDeflectNode(uint32_t dstId, uint32_t routeNextId);

	double UpdateQvalue(uint32_t dstId, double reward, uint32_t HopCount);
	void UpdateRoute(uint32_t dstId, uint32_t nextId, double Qvalue, uint32_t HopCount);
	//void AddPreNode();
	//uint32_t LookupPreNode();the algorithm did not need to lookup some node, these will be finished at upper layer.
	void AddPreNodeNew(uint32_t distId, uint32_t nextID, double Qvalue, uint32_t HopCount);
	uint32_t UpdatePreNode(uint32_t dstId, uint32_t nextId, double reward, uint32_t HopCount);

	typedef struct {
		uint32_t dstId;
		uint32_t nextId;
		double Qvalue;
		double RecRate;
		uint32_t HopCount;
		double UpTime; //
		bool RouteValid;//when a node is using this route, it will become false.
	} QTableEntry;


	typedef struct{
		uint32_t dstId;
		uint32_t nodeId;
		uint32_t nodeType;
		double Qvalue;
		double RecRate;
		uint32_t HopCount;
		double UpTime;
	} PreNodeEntry;

	std::vector<QTableEntry> m_qtable;
	std::vector<PreNodeEntry> m_prenode;

public:
	//virtual void SendPacket(Ptr<Packet> p); //调用nano-routing-entity中的虚函数，其中nano-routing-entity中的为纯虚函数，在这里进行实现。
	virtual void ReceivePacket(Ptr<Packet> p);
	//virtual void ForwardPacket(Ptr<Packet> p);
	//virtual void ForwardPacket(Ptr<Packet> p, uint32_t fromNodeId);
//send feedback
public:
	virtual void SendPacketDst(Ptr<Packet> p, uint32_t dstNodeId);
	void SendFeedbackPacket(Ptr<Packet> p, uint32_t fromNodeId);
	virtual void ForwardPacket(Ptr<Packet> p, uint32_t fromNodeId);
	//checke the sent/received that whethe the packet has been sent/received before.
public:
	void UpdateReceivedPacketId(uint32_t id);
	bool CheckAmongReceivedPacket(uint32_t id);
	void SetReceivedPacketListDim(int m);

	void UpdateSentPacketId(uint32_t id, uint32_t nextHop);
	bool CheckAmongSentPacket(uint32_t id,uint32_t nextHop);
	void SetSentPacketListDim(int m);

private:

	std::list<uint32_t> m_receivedPacketList;
	int m_receivedPacketListDim;
	std::list<std::pair<uint32_t, uint32_t>> m_sentPacketList;
	int m_sentPacketListDim;

};
}

#endif /* SRC_NANONETWORKS_MODEL_Q_ROUTING_H_ */
