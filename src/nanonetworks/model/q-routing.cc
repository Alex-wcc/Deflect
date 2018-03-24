/*
 * q-routing.cc
 *
 *  Created on: 2018-3-16
 *      Author: eric
 */

#include "random-nano-routing-entity.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/packet.h"
#include "simple-nano-device.h"
#include "nano-mac-queue.h"
#include "nano-l3-header.h"
#include "nano-mac-entity.h"
#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/enum.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/channel.h"
#include "simple-nano-device.h"
#include "nano-spectrum-phy.h"
#include "nano-mac-entity.h"
#include "nano-mac-header.h"
#include "nano-seq-ts-header.h"
#include "ns3/simulator.h"
#include "nano-routing-entity.h"
#include "message-process-unit.h"
#include <stdio.h>
#include <stdlib.h>

#include "ns3/nstime.h"
#include <algorithm>
#include <boost/bind.hpp>
#include "q-routing.h"

NS_LOG_COMPONENT_DEFINE("QRouting");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(QRouting);

TypeId QRouting::GetTypeId() {
	static TypeId tid = TypeId("ns3::QRouting").SetParent<Object>();
	return tid;
}

QRouting::QRouting() {
	SetDevice(0);
	m_receivedPacketListDim = 20;
	for (int i = 0; i < m_receivedPacketListDim; i++) {
		m_receivedPacketList.push_back(9999999);
	}
	m_sentPacketListDim = 20;
	for (int i = 0; i < m_sentPacketListDim; i++) {
		std::pair<uint32_t, uint32_t> item;
		item.first = 9999999;
		item.second = 9999999;
		m_sentPacketList.push_back(item);
	}
	//Simulator::Schedule(Seconds(0.001), &QRouting::AddPreNode, this);
	//需要对m_qtable 和m_prenode 进行初始化，初始化后，查表要判断是不是自己，如果是自己，就不发送
	QTableEntry OwnTable;
	PreNodeEntry OwnPreNode;
	OwnTable.dstId = 999;
	m_qtable.push_back(OwnTable);
	OwnPreNode.dstId = 999;
	m_prenode.push_back(OwnPreNode);
}

QRouting::~QRouting() {
	SetDevice(0);
}

void QRouting::DoDispose() {
	SetDevice(0);
}

void QRouting::AddRoute(uint32_t dstId, uint32_t nextId, uint32_t Qvalue,
		uint32_t HopCount) {
	//if the route is already there, it will not be added.
	//but upper layer will charge this.
	NS_LOG_FUNCTION(
			"Addroute:" << dstId << "nextId:" << nextId << "Qvalue:" << Qvalue << "HopCount:" << HopCount);
	QTableEntry Qroute;
	Qroute.dstId = dstId;
	Qroute.nextId = nextId;
	Qroute.Qvalue = Qvalue + 0.1 * Qvalue;	//初始值因为增加了一跳相当于，随意Q值需要进行增大。
	Qroute.RecRate = 0.1;
	Qroute.HopCount = HopCount;
	Qroute.UpTime = Simulator::Now().GetSeconds();
	;
	Qroute.RouteValid = true;

	m_qtable.push_back(Qroute);
}

void QRouting::AddPreNodeNew(uint32_t dstId, uint32_t nextId, uint32_t Qvalue,
		uint32_t HopCount) {
	NS_LOG_FUNCTION(
			"AddPreNode: dstId:"<<dstId<<"nextId:"<<nextId<<"Qvalue"<<Qvalue<<"HopCount"<<HopCount);
	PreNodeEntry PreNode;
	PreNode.dstId = dstId;
	PreNode.nodeId = nextId;
	PreNode.nodeType = 0;
	PreNode.Qvalue = Qvalue + 0.1 * Qvalue; //初始值因为增加了一跳相当于，随意Q值需要进行增大。
	PreNode.HopCount = HopCount;
	PreNode.RecRate = 0.1;
	PreNode.UpTime = Simulator::Now().GetSeconds();
	m_prenode.push_back(PreNode);
}
//route look up
uint32_t QRouting::LookupRoute(uint32_t dstId) {
	std::vector<QTableEntry>::iterator it = m_qtable.begin();

	uint16_t i = 0; //count number
	for (; it != m_qtable.end(); ++it) {
		if (it->dstId == dstId
				&& it->dstId != 999/*&& it->RouteValid == true*/) {
			break;
		}
		i++;
	}
	if (i < m_qtable.size()) {
		QTableEntry Qroute = m_qtable.at(i);
		return Qroute.nextId;
	} else {
		return 991; //不存在路由
	}
}
void QRouting::SetRouteUn(uint32_t nextId) {
	std::vector<QTableEntry>::iterator it = m_qtable.begin();

	uint16_t i = 0; //count number
	for (; it != m_qtable.end(); ++it) {
		if (it->nextId == nextId
				&& it->dstId != 999/*&& it->RouteValid == true*/) {

			it->RouteValid = false;
		}
		i++;
	}
}
void QRouting::SetRouteAv(uint32_t nextId) {
	std::vector<QTableEntry>::iterator it = m_qtable.begin();

	uint16_t i = 0; //count number
	for (; it != m_qtable.end(); ++it) {
		if (it->nextId == nextId
				&& it->dstId != 999/*&& it->RouteValid == true*/) {
			it->RouteValid = true;
		}
		i++;
	}
}
//search route for the q value, add into the header then next node can update its routing table and q table
uint32_t QRouting::SearchRouteForQvalue(uint32_t dstId) {
	std::vector<QTableEntry>::iterator it = m_qtable.begin();

	uint16_t i = 0; //count number
	for (; it != m_qtable.end(); ++it) {
		if (it->dstId == dstId
				&& it->dstId != 999/*&& it->RouteValid == true*/) {
			break;
		}
		i++;
	}
	if (i < m_qtable.size()) {
		QTableEntry Qroute = m_qtable[i];
		return Qroute.Qvalue;
	} else {
		return 992; //不存在route，没有qvalue
	}
}
//search route for the routing hop count, add into the header then next node can update its routing talbe and q table
uint32_t QRouting::SearchRouteForQHopCount(uint32_t dstId) {
	std::vector<QTableEntry>::iterator it = m_qtable.begin();

	uint16_t i = 0; //count number
	for (; it != m_qtable.end(); ++it) {
		if (it->dstId == dstId
				&& it->dstId != 999/*&& it->RouteValid == true*/) {
			break;
		}
		i++;
	}
	if (i < m_qtable.size()) {
		QTableEntry Qroute = m_qtable[i];
		return Qroute.HopCount;
	} else {
		return 993; //不存在route，且没有qhopcount
	}
}

//route available?? 可以和lookuproute合并
bool QRouting::RouteAvailable(uint32_t dstId) {
	std::vector<QTableEntry>::iterator it = m_qtable.begin();

	uint16_t i = 0;
	for (; it != m_qtable.end(); ++it) {
		if (it->dstId == dstId && it->RouteValid == true && it->dstId != 999) {
			break;
		}
		i++;
	}
	//QTableEntry Qroute = m_qtable[i];
	if (i < m_qtable.size()) {
		return true;
	} else {
		return false;
	}
}
//preNode Look up搜索有没有邻居节点可以进行发送的。
uint32_t QRouting::LookUpPreNode(uint32_t dstId) {
	std::vector<PreNodeEntry>::iterator it = m_prenode.begin();

	uint16_t i = 0;
	for (; it != m_prenode.end(); ++it) {
		if (it->nodeId == dstId && it->dstId != 999) {
			break;
		}
		i++;
	}
	if (i < m_prenode.size()) {
		PreNodeEntry PreNode = m_prenode[i];
		return PreNode.nodeId;
	} else {
		return 994; //不存在prenode
	}
}
//选择一个deflect node
uint32_t QRouting::ChooseDeflectNode(uint32_t dstId, uint32_t routeNextId) {
	std::vector<PreNodeEntry>::iterator it = m_prenode.begin();

	uint16_t i = 0;
	uint16_t j = 0; //用于比大小
	//PreNodeEntry PreNode;
	//double qvalue;
	PreNodeEntry cPreNode;
	//double cqvalue;
	double deltaT = Simulator::Now().GetSeconds();

	for (; it != m_prenode.end(); ++it) {
		if (it->dstId == dstId && it->nodeId != routeNextId) {
			j = i;
			break;
		}
		i++;
	}
	cPreNode = m_prenode[j];
	//cqvalue = cPreNode.Qvalue;

	for (int i = 0; it != m_prenode.end(); ++it) {
		if (it->dstId == dstId && it->nodeId != routeNextId) {

			if (it->Qvalue + (deltaT - it->UpTime) * it->RecRate
					< cPreNode.Qvalue
							+ (deltaT - cPreNode.UpTime) * cPreNode.RecRate) { // 寻找最小值
				j = i;
				cPreNode = m_prenode[j];
			}
		}
		i++;
	}

	//PreNode = m_prenode.at(i);
	//cPreNode = m_prenode.at(j);
	if (i < m_prenode.size()) {
		return cPreNode.nodeId;
	} else {
		return 995;	//不存在deflectnode
	}
}
//搜索记录存在不存在，不存在这样的记录就增加
bool QRouting::SearchPreNode(uint32_t dstId, uint32_t nextId) {
	std::vector<PreNodeEntry>::iterator it = m_prenode.begin();

	uint16_t i = 0;
	for (; it != m_prenode.end(); ++it) {
		if (it->dstId == dstId && it->nodeId == nextId && it->dstId != 999) {
			break;
		}
		i++;
	}
	//PreNodeEntry PreNode = m_prenode[i];
	if (i < m_prenode.size()) {
		return true;
	} else {
		return false;
	}
}

//update the q talbe of a route
uint32_t QRouting::UpdateQvalue(uint32_t dstId, double preReward,
		uint32_t HopCount) {
	std::vector<QTableEntry>::iterator it = m_qtable.begin();

	uint16_t i;
	for (; it != m_qtable.end(); ++it) {
		if (it->dstId == dstId && it->dstId != 999) {
			break;
		}
		i++;
	}
	if (i < m_qtable.size()) {
		QTableEntry Qroute = m_qtable[i];
		double Reward = preReward / Qroute.HopCount - Qroute.Qvalue;
		double Timenow = Simulator::Now().GetSeconds();
		//update the q value
		Qroute.Qvalue = Qroute.Qvalue + 0.1 * Reward; //!!!! here 0.1 means the learning rate, we can revised it !!!!
		//update the hop count
		Qroute.HopCount = HopCount + 1;
		//update the recovery rate
		if (Reward < 0) {
			Qroute.RecRate = Qroute.RecRate
					+ 0.1 * Reward / (Timenow - Qroute.UpTime);
			//we should careful with the Reward and time
			//we will revised it
			//here 0.1 is the recovery learning rate
		} else {
			Qroute.RecRate = 0.9 * Qroute.RecRate;
			//here 0.9 is the decay rates
			//we should take care of this.
			//we will revise it.
		}
		//update the update time
		Qroute.UpTime = Timenow;
		return Qroute.Qvalue;
	} else {
		return 996;
	}
}

void QRouting::UpdatePreNode(uint32_t dstId, uint32_t nextId, uint32_t reward,
		uint32_t HopCount) {
	std::vector<PreNodeEntry>::iterator it = m_prenode.begin();

	uint16_t i = 0;
	for (; it != m_prenode.end(); ++it) {
		if (it->dstId == dstId && it->nodeId == nextId && it->dstId != 999) {
			break;
		}
		i++;
	}
	if (i < m_prenode.size()) {
		PreNodeEntry PreNode = m_prenode[i];
		double Reward = reward / PreNode.HopCount + 0.1 - PreNode.Qvalue;
		double Timenow = Simulator::Now().GetSeconds();
		PreNode.Qvalue = PreNode.Qvalue + 0.1 * Reward;
		PreNode.HopCount = HopCount + 1;
		if (Reward < 0) {
			PreNode.RecRate = PreNode.RecRate
					+ 0.1 * Reward / (Timenow - PreNode.UpTime);
		} else {
			PreNode.RecRate = 0.9 * PreNode.RecRate;
		}
		PreNode.UpTime = Timenow;
	}
}

//update the routing table
void QRouting::UpdateRoute(uint32_t dstId, uint32_t nextId, uint32_t Qvalue,
		uint32_t HopCount) {
	uint16_t i; //count number
	std::vector<QTableEntry>::iterator it = m_qtable.begin();

	for (; it != m_qtable.end(); ++it) {
		if (it->dstId == dstId && it->dstId != 999) {
			break;
		}
		i++;
	}
	if (i < m_qtable.size()) {
		QTableEntry OldQroute = m_qtable[i];
		if (OldQroute.Qvalue > Qvalue) {
			OldQroute.dstId = dstId;
			OldQroute.nextId = nextId;
			OldQroute.Qvalue = Qvalue;
			OldQroute.HopCount = HopCount;
			OldQroute.RecRate = 0.1;
			OldQroute.UpTime = Simulator::Now().GetSeconds();
			OldQroute.RouteValid = true;
		}
	}
}
void QRouting::SendPacket(Ptr<Packet> p) {
} //纯虚函数，但是也不定义，另外一个做法，在这里进行随机数的选择。
//可以进行精简，把三个if相同的部分取出来
void QRouting::SendPacketDst(Ptr<Packet> p, uint32_t dstNodeId) {
	NS_LOG_FUNCTION(this<<p<<"size"<<p->GetSize());

	NanoSeqTsHeader seqTs;
	p->RemoveHeader(seqTs);
	NS_LOG_FUNCTION(this<<p<<"size"<<p->GetSize()<<seqTs);

	uint32_t dst = dstNodeId; //目的地址
	uint32_t nextId = GetDevice()->GetNode()->GetId(); //如果找不到就给自己。

	uint32_t routenextId = LookupRoute(dst);
	bool routeava = RouteAvailable(dst);
	uint32_t deflectId;
	if (routenextId != 991) {
		deflectId = ChooseDeflectNode(dst, routenextId);
	}
	if (deflectId != 995) {
		routenextId = LookupRoute(dst);
	}

	if (routenextId != 991 && routeava) { //如果目的地址不是邻居节点，但是在路由表上
		//获取邻居节点号  路由表发送
		nextId = LookupRoute(dst);
		NS_LOG_FUNCTION(this<<"There is a route" << "nextId" << nextId);
	} else if (routenextId != 991 && !routeava && deflectId != 995) {

		nextId = deflectId;
		NS_LOG_FUNCTION(this<<"The packet is deflected"<<"nextId"<<nextId);
	} else {		//没有可用的端口,随机发送一个到别的端口
//这里不能是路由端口，或者不能是已经在使用的端口，这里需要进行修改。
		std::vector<std::pair<uint32_t, uint32_t>> neighbors =
				GetDevice()->GetMac()->m_neighbors;
		if (neighbors.size() != 0) {
			NS_LOG_FUNCTION(
					this<<p<<"no route, random choose, neighbors.size () > 0, try to select a nanonode");
			srand(time(NULL));
			int i = rand() % neighbors.size();
			nextId = neighbors[i].first;
		}
		NS_LOG_FUNCTION(this<<"random choose a neighbor" << "nextId" << nextId);
	}

	NanoL3Header header;
	uint32_t src = GetDevice()->GetNode()->GetId();
	uint32_t id = seqTs.GetSeq();
	uint32_t ttl = 100;
	uint32_t qvalue = 100;		//开始发送的时候，没有Q值。因为是自己。
	uint32_t hopcount = 1;
	header.SetSource(src);
	header.SetDestination(dst);
	header.SetTtl(ttl);
	header.SetPacketId(id);
	header.SetQvalue(qvalue);
	header.SetHopCount(hopcount);
	header.SetPrevious(src);
	header.SetQHopCount(0);
	p->AddHeader(seqTs);
	p->AddHeader(header);
	NS_LOG_FUNCTION(this<<p<<"size"<<p->GetSize());

	UpdateSentPacketId(id, nextId);

	Ptr<NanoMacEntity> mac = GetDevice()->GetMac();
	//这里需要将路由表置位
	SetRouteUn(nextId);
	mac->Send(p, nextId);
	SetRouteAv(nextId);
	//这里需要将路由表置位

}

void QRouting::ReceivePacket(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this<<p<<"size"<<p->GetSize());

	NanoMacHeader macHeader;
	p->RemoveHeader(macHeader);
	uint32_t macfrom = macHeader.GetSource();
	uint32_t macto = macHeader.GetDestination();

	NanoL3Header l3Header;
	p->RemoveHeader(l3Header);	//这里删除了，后面就需要进行增加。

	NS_LOG_FUNCTION(this<<macHeader);
	NS_LOG_FUNCTION(this<<"my id"<<GetDevice()->GetNode()->GetId());

	if (macto == GetDevice()->GetNode()->GetId()) {
		NS_LOG_FUNCTION(this<<"is for me");
		//GetDevice()->GetMessageProcessUnit()->ProcessMessage(p);

		uint32_t from = l3Header.GetSource();
		uint32_t to = l3Header.GetDestination();
		uint32_t previous = l3Header.GetPrevious();
		uint32_t qvalue = l3Header.GetQvalue();
		uint32_t hopcount = l3Header.GetHopCount();
		uint32_t qhopcount = l3Header.GetQHopCount();

		uint32_t thisid = GetDevice()->GetNode()->GetId();

		bool PreNodeE = SearchPreNode(from, previous);
		if (hopcount == 1) {
			if (!PreNodeE && previous != thisid) {
				//add the q value table, i.e. the prenode
				AddPreNodeNew(from, previous, qvalue, hopcount);
				uint32_t route = LookupRoute(from);
				if (route == 991 && previous != thisid) {//路由表中已经有记录了，就不行进行增加了
					AddRoute(from, previous, qvalue, hopcount);
				}
				//AddRoute(from, previous, qvalue, hopcount);
			} else {
				//也就是存在一跳的邻居节点。也就不用更新了，因为只有一跳。
			}
		} else {		//跳数大于一跳
			if (!PreNodeE && previous != thisid) {
				AddPreNodeNew(from, previous, qvalue, hopcount);
				uint32_t route = LookupRoute(from);
				if (route == 991 && previous != thisid) {//路由表中已经有记录了，就不行进行增加了
					AddRoute(from, previous, qvalue, hopcount);
				}

			} else if (PreNodeE && previous != thisid) {
				//
				//对routingtable 进行更新
				uint32_t preReward = qvalue * (qhopcount + 1);
				UpdatePreNode(from, previous, preReward, qhopcount);
				uint32_t newQvalue = UpdateQvalue(from, preReward, qhopcount);//更新路由表上的Q值
				if (newQvalue == 996) {
				} else {
					UpdateRoute(from, previous, newQvalue, qhopcount);
				}
			}
		}
		if (to == GetDevice()->GetNode()->GetId()) {
			NS_LOG_FUNCTION(this<<"i am the destination");
			p->AddHeader(l3Header);
			GetDevice()->GetMessageProcessUnit()->ProcessMessage(p);
		} else {
			p->AddHeader(l3Header);
			NS_LOG_FUNCTION(this<<"forward!");
			ForwardPacket(p, macfrom);
		}
	}
}

void QRouting::ForwardPacket(Ptr<Packet> p) {
}

void QRouting::ForwardPacket(Ptr<Packet> p, uint32_t macfrom) {
	NS_LOG_FUNCTION(this);
	uint32_t nextId = GetDevice()->GetNode()->GetId();	//如果找不到就给自己。

	NanoL3Header l3Header;
	p->RemoveHeader(l3Header);
	uint32_t id = l3Header.GetPacketId();
	NS_LOG_FUNCTION(this<<l3Header);
	NS_LOG_FUNCTION(this<<"packet id"<< id<< "from"<<macfrom);

	uint32_t from = l3Header.GetSource();
	uint32_t to = l3Header.GetDestination();
	uint32_t headerQvalue = l3Header.GetQvalue();	//
	uint32_t hopcount = l3Header.GetHopCount();	//
	uint32_t ttl = l3Header.GetTtl();
	uint32_t qhopcount = l3Header.GetQHopCount() + 1;

	uint32_t routenextId = LookupRoute(to);
	bool routeava = RouteAvailable(to);
	uint32_t deflectedId = ChooseDeflectNode(to, routenextId);

	if (deflectedId != 995) {
		routenextId = LookupRoute(to);
	}
	uint32_t thisid = GetDevice()->GetNode()->GetId();
	if (ttl > 1) {
		if (routenextId != 991 && routeava) {
			nextId = routenextId;
			NS_LOG_FUNCTION(this<<"Forward there is route"<<"nextId"<<nextId);

			// 获得 from ，previous 对应的qvalue 和hopcount；
			headerQvalue = SearchRouteForQvalue(from);
			qhopcount = SearchRouteForQHopCount(from);
		} else if (routenextId != 991 && !routeava && deflectedId != 995) {
			nextId = deflectedId;
			NS_LOG_FUNCTION(
					this<<"Forward the packet is deflected"<<"nextId"<<nextId);

			// 获得 from ，previous 对应的qvalue 和hopcount；
			headerQvalue = SearchRouteForQvalue(from);
			qhopcount = SearchRouteForQHopCount(from);
		} else {
			std::vector<std::pair<uint32_t, uint32_t>> neighbors =
					GetDevice()->GetMac()->m_neighbors;
			if (neighbors.size() != 0) {
				NS_LOG_FUNCTION(
						this<<"no route, random choose, neighbors.size()>0,try to select a nanonode");
				srand(time(NULL));
				int i = rand() % neighbors.size();
				nextId = neighbors[i].first;
			}
			NS_LOG_FUNCTION(this<<"random choose a neighbor"<<"nextId"<<nextId);
		}
		l3Header.SetTtl(ttl - 1);
		l3Header.SetHopCount(hopcount + 1);
		l3Header.SetPrevious(thisid);
		l3Header.SetQvalue(headerQvalue);
		l3Header.SetQHopCount(qhopcount);		//将搜索到的路由表中的跳数用于q值的更新。
		NS_LOG_FUNCTION(this<<"forward new l3 header"<<l3Header);
		p->AddHeader(l3Header);
		Ptr<NanoMacEntity> mac = GetDevice()->GetMac();
		//这里将路由表置位
		SetRouteUn(nextId);
		mac->Send(p, nextId);
		SetRouteAv(nextId);
		//这里将路由表置位
	} else {
		NS_LOG_FUNCTION(this<<"ttl expired");
	}
}

void QRouting::UpdateReceivedPacketId(uint32_t id) {
	NS_LOG_FUNCTION(this);
	m_receivedPacketList.pop_front();
	m_receivedPacketList.push_back(id);
}

bool QRouting::CheckAmongReceivedPacket(uint32_t id) {
	NS_LOG_FUNCTION(this);
	for (std::list<uint32_t>::iterator it = m_receivedPacketList.begin();
			it != m_receivedPacketList.end(); ++it) {
		NS_LOG_FUNCTION(this<<*it<<id);
		if (*it == id)
			return true;
	}
	return false;
}

void QRouting::SetReceivedPacketListDim(int m) {
	NS_LOG_FUNCTION(this);
	m_receivedPacketListDim = m;
}

void QRouting::UpdateSentPacketId(uint32_t id, uint32_t nextHop) {
	NS_LOG_FUNCTION(this<<id<<nextHop);
	m_sentPacketList.pop_front();
	std::pair<uint32_t, uint32_t> item;
	item.first = id;
	item.second = nextHop;
	m_sentPacketList.push_back(item);
}

bool QRouting::CheckAmongSentPacket(uint32_t id, uint32_t nextHop) {
	NS_LOG_FUNCTION(this);

	for (std::list<std::pair<uint32_t, uint32_t> >::iterator it =
			m_sentPacketList.begin(); it != m_sentPacketList.end(); it++) {
		if ((*it).first == id && (*it).second == nextHop)
			return true;
	}
	return false;
}

void QRouting::SetSentPacketListDim(int m) {
	NS_LOG_FUNCTION(this);
	m_sentPacketListDim = m;
}
}
