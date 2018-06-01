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
//#include "ns3/callback.h"
//#include "ns3/traced-callback.h"

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
	//05/30/2018 因为energy prediction 增加两个变量.  增加一个关于能量状态的变量
	void AddRoute(uint32_t dstId, uint32_t nextId, uint32_t Qvalue,
			uint32_t HopCount, double energyharvestspeed,
			double energyconsumespeed, double energyconsumerate);
	uint32_t LookupRoute(uint32_t dstId);
	uint32_t SearchRouteForQvalue(uint32_t dstId);
	uint32_t SearchRouteForQHopCount(uint32_t dstId);
	bool RouteAvailable(uint32_t dstId);
	uint32_t LookUpPreNode(uint32_t dstId);
	bool SearchPreNode(uint32_t dstId, uint32_t nextId); //寻找对应的记录是否存在，以便于进行添加或者更新
	uint32_t ChooseDeflectNode(uint32_t dstId, uint32_t routeNextId);
	//05/31/2018 因为energy prediction 增加两个变量.  增加一个关于能量状态的变量
	uint32_t UpdateQvalue(uint32_t dstId, double reward, uint32_t HopCount,
			double energyharvestspeed, double energyconsumespeed,
			double energyconsumerate);
	void UpdateRoute(uint32_t dstId, uint32_t nextId, uint32_t Qvalue,
			uint32_t HopCount);
	//void AddPreNode();
	//uint32_t LookupPreNode();the algorithm did not need to lookup some node, these will be finished at upper layer.
	//05/30/2018 因为energy prediction 增加两个变量. 增加一个关于能量状态的变量
	void AddPreNodeNew(uint32_t distId, uint32_t nextID, uint32_t Qvalue,
			uint32_t HopCount, double energyharvestspeed,
			double energyconsumespeed, double energyconsumerate);
	//05/30/2018 因为energy prediction 增加两个变量.  增加一个关于能量状态的变量
	void UpdatePreNode(uint32_t dstId, uint32_t nextId, double reward,
			uint32_t HopCount, double energyharvestspeed,
			double energyconsumespeed, double energyconsumerate);

	void SetRouteUn(uint32_t nextId);
	void SetRouteAv(uint32_t nextId);
	//send the ack packet
	void SendACKPacket(uint32_t fromId);
	void SendACKFeedback(uint32_t destination, uint32_t macfrom, uint32_t qvalue, uint32_t hopcount,
			uint32_t nextid, uint32_t deflectrate, uint32_t droprate,
			uint32_t energyrate);

	typedef struct {
		uint32_t dstId;
		uint32_t nextId;
		uint32_t Qvalue;
		double RecRate;
		uint32_t HopCount;
		double UpTime; //
		bool RouteValid; //when a node is using this route, it will become false.
		//for energy prediction
		double energyharvestspeed;
		double energyconsumespeed;
		double energyconsumerate;
	} QTableEntry;

	typedef struct {
		uint32_t dstId;
		uint32_t nodeId;
		uint32_t nodeType;
		uint32_t Qvalue;
		double RecRate;
		uint32_t HopCount;
		double UpTime;
		bool PreNodeValid;
		//for energy prediction
		double energyharvestspeed;
		double energyconsumespeed;
		double energyconsumerate;
	} PreNodeEntry;

	std::vector<QTableEntry> m_qtable;
	std::vector<PreNodeEntry> m_prenode;

public:
	virtual void SendPacket(Ptr<Packet> p); //调用nano-routing-entity中的虚函数，其中nano-routing-entity中的为纯虚函数，在这里进行实现。
	virtual void ReceivePacket(Ptr<Packet> p);
	virtual void ForwardPacket(Ptr<Packet> p);
	//virtual void ForwardPacket(Ptr<Packet> p, uint32_t fromNodeId);
//send feedback
public:
	virtual void SendPacketDst(Ptr<Packet> p, uint32_t dstNodeId);
	void SendFeedbackPacket(Ptr<Packet> p, uint32_t fromNodeId);
	void ForwardPacket1(Ptr<Packet> p, uint32_t fromNodeId);
	void SendPacketBuf();
	//checke the sent/received that whethe the packet has been sent/received before.
public:
	void UpdateReceivedPacketId(uint32_t id);
	bool CheckAmongReceivedPacket(uint32_t id);
	void SetReceivedPacketListDim(int m);

	void UpdateSentPacketId(uint32_t id, uint32_t nextHop);
	bool CheckAmongSentPacket(uint32_t id, uint32_t nextHop);
	void SetSentPacketListDim(int m);

	void UpdateToSendBuffer(Ptr<Packet> p);
	void UpdateMacFrom(uint32_t id, uint32_t macfrom);
	uint32_t SearchMacFrom(uint32_t id);

private:

	std::list<uint32_t> m_receivedPacketList;
	int m_receivedPacketListDim;
	std::list<std::pair<uint32_t, uint32_t>> m_sentPacketList;
	int m_sentPacketListDim;
	int m_sendBuffer; //in this case the buffer = 1,no use

	std::list<std::pair<Ptr<Packet>, int>> m_tosendBuffer;
	std::list<std::pair<uint32_t, uint32_t>> m_macFrom;

	bool m_SendingTag; //正在发送的标记位

	//能量限制；
	/*double SendPacketRecACK;
	 double RecPacketForward;
	 double RecACK;*/

	//outstream
	//typedef void (*OutTxCallback)(int, int, int);
	//TracedCallback<int, int, int> m_SendTx;
};
}

#endif /* SRC_NANONETWORKS_MODEL_Q_ROUTING_H_ */
