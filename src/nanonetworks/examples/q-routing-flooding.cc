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
	m_sendBuffer = 0;	//因为没有初始化，所以在这里进行初始化。
	m_SendingTag = false;	//正在发送的标记位，初始化位false
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
	PreNode.PreNodeValid = true;
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

	std::vector<PreNodeEntry>::iterator itprenode = m_prenode.begin();
	uint16_t j = 0;
	for (; itprenode != m_prenode.end(); ++itprenode) {
		if (itprenode->nodeId == nextId && itprenode->nodeId != 999) {
			itprenode->PreNodeValid = false;
		}
		j++;
	}
	//0.1秒后，进行重置，但是这里的 秒需要根据能量捕获速率来进行调整。
	Simulator::Schedule(Seconds(0.1), &QRouting::SetRouteAv, this, nextId);
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
	std::vector<PreNodeEntry>::iterator itprenode = m_prenode.begin();
	uint16_t j = 0;
	for (; itprenode != m_prenode.end(); ++itprenode) {
		if (itprenode->nodeId == nextId && itprenode->nodeId != 999) {
			itprenode->PreNodeValid = true;
		}
		j++;
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
		if (it->nodeId == dstId && it->dstId != 999
				&& it->PreNodeValid == true) {
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
		if (it->dstId == dstId && it->nodeId != routeNextId
				&& it->PreNodeValid == true) {
			j = i;
			break;
		}
		i++;
	}
	cPreNode = m_prenode[j];
	//cqvalue = cPreNode.Qvalue;

	for (int i = 0; it != m_prenode.end(); ++it) {
		if (it->dstId == dstId && it->nodeId != routeNextId
				&& it->PreNodeValid == true) {

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

void QRouting::UpdatePreNode(uint32_t dstId, uint32_t nextId, double reward,
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

		double Reward = reward / PreNode.HopCount - PreNode.Qvalue;
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

	uint16_t i = 0; //count number
	std::vector<QTableEntry>::iterator it = m_qtable.begin();
	std::vector<PreNodeEntry>::iterator nodeit = m_prenode.begin();

	for (; it != m_qtable.end(); ++it) {
		if (it->dstId == dstId && it->dstId != 999) {
			break;
		}
		i++;
	}
	if (i < m_qtable.size()) {
		QTableEntry OldQroute = m_qtable[i];
		for (; nodeit != m_prenode.end(); ++nodeit) {
			if (nodeit->dstId == dstId && nodeit->dstId != 999) {
				if (OldQroute.Qvalue > nodeit->Qvalue) {
					m_qtable[i].dstId = dstId;
					m_qtable[i].nextId = nodeit->nodeId;
					m_qtable[i].Qvalue = nodeit->Qvalue;
					m_qtable[i].HopCount = nodeit->HopCount;
					m_qtable[i].RecRate = 0.1;
					m_qtable[i].UpTime = Simulator::Now().GetSeconds();
					m_qtable[i].RouteValid = true;
				}
			}
		}
	}
}
void QRouting::SendPacket(Ptr<Packet> p) {
} //纯虚函数，但是也不定义，另外一个做法，在这里进行随机数的选择。
//可以进行精简，把三个if相同的部分取出来
void QRouting::SendPacketDst(Ptr<Packet> p, uint32_t dstNodeId) {
	NS_LOG_FUNCTION(this<<p<<"size"<<p->GetSize());

	if (GetDevice()->m_energy < 21
			|| m_tosendBuffer.size() >= GetDevice()->m_bufferSize) {
		//send
		int energy = GetDevice()->m_energy;
		energy = energy + 1;
		int queue = m_tosendBuffer.size();
		queue = queue + 1;
	} else if (GetDevice()->m_energy >= 21
			&& m_tosendBuffer.size() < GetDevice()->m_bufferSize) {
		NanoSeqTsHeader seqTs;
		p->RemoveHeader(seqTs);
		NS_LOG_FUNCTION(this<<p<<"size"<<p->GetSize()<<seqTs);

		uint32_t dst = dstNodeId; //目的地址
		uint32_t nextId = 999; //GetDevice()->GetNode()->GetId(); //如果找不到就给自己。

		uint32_t routenextId = LookupRoute(dst);
		bool routeava = RouteAvailable(dst);
		uint32_t deflectId;
		if (routenextId != 991) {
			deflectId = ChooseDeflectNode(dst, routenextId);
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
				srand(time(NULL));		//这个随机数？？？？
				int i = rand() % neighbors.size();
				nextId = neighbors[i].first;
			}
			NS_LOG_FUNCTION(
					this<<"random choose a neighbor" << "nextId" << nextId);
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
		header.SetDeflectRate(0);
		header.SetDropRate(0);
		header.SetEnergyRate(0);
		p->AddHeader(seqTs);
		p->AddHeader(header);

		NS_LOG_FUNCTION(this<<p<<"size"<<p->GetSize());

		//GetDevice()->m_queuePacket.push_back(p);
		if (m_SendingTag == true) {
			UpdateToSendBuffer(p);
		} else {
			UpdateToSendBuffer(p);
			SendPacketBuf();
		}

	}
}

void QRouting::ReceivePacket(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this<<p<<"size"<<p->GetSize());

	NanoMacHeader macHeader;
	p->RemoveHeader(macHeader);
	uint32_t macfrom = macHeader.GetSource();
	uint32_t macto = macHeader.GetDestination();

	NS_LOG_FUNCTION(this<<macHeader);
	NS_LOG_FUNCTION(this<<"my id"<<GetDevice()->GetNode()->GetId());

	if (p->GetSize() < 102)	//因为一个正常的包的大小是102bytes，所以小于这个大小的时候就是ACK
			{
		if (macto == GetDevice()->GetNode()->GetId()) {
			NS_LOG_FUNCTION(this<<"is for me");
			if (GetDevice()->m_energy < 1) {
				int energy = GetDevice()->m_energy;
				energy = energy + 1;
			} else {
				//接收的ACK进行+1的操作
				GetDevice()->m_ReceiveACKCount = GetDevice()->m_ReceiveACKCount
						+ 1;
				m_tosendBuffer.remove(m_tosendBuffer.front());
				//consume energy
				GetDevice()->ConsumeEnergyRecACK();
				//NS_LOG_FUNCTION(this<<'receive the ack'<<p<<'size'<<p->GetSize());
				SetRouteAv(macfrom);
			}
		}
	} else {
		if (GetDevice()->m_energy <= 32
				|| m_tosendBuffer.size() >= GetDevice()->m_bufferSize) {
			//receive
			int energy = GetDevice()->m_energy;
			energy = energy + 1;
			int queue = m_tosendBuffer.size();
			queue = queue + 1;
		}
		if (GetDevice()->m_energy > 32
				&& m_tosendBuffer.size() < GetDevice()->m_bufferSize) {
			GetDevice()->ConsumeEnergyRecPacket();
			SendACKPacket(macfrom);
			GetDevice()->ConsumeEnergySendACK();

			//接收的数据包进行+1操作
			GetDevice()->m_ReceiveCount = GetDevice()->m_ReceiveCount + 1;
			NanoL3Header l3Header;
			p->RemoveHeader(l3Header);	//这里删除了，后面就需要进行增加。


			uint32_t to = l3Header.GetDestination();
			uint32_t packetid = l3Header.GetPacketId();

			bool alreadyReceived = CheckAmongReceivedPacket(packetid);
			UpdateReceivedPacketId (packetid);

//下面这个函数打印节点的状态，在收到一个数据包后进行打印
			GetDevice()->GetMessageProcessUnit()->PrintNodestatus();
			//update the q routing table and prenode table

			if (!alreadyReceived) {
				if (to == GetDevice()->GetNode()->GetId()) {
					NS_LOG_FUNCTION(this<<"i am the destination");
					p->AddHeader(l3Header);
					GetDevice()->GetMessageProcessUnit()->ProcessMessage(p);
				} else {
					p->AddHeader(l3Header);
					NS_LOG_FUNCTION(this<<"forward!");
					ForwardPacket1(p, macfrom);
				}
			}
			else
			{
				NS_LOG_FUNCTION (this << "packet already received in the past");
			}
		}
	}

}

void QRouting::SendACKPacket(uint32_t fromId) {
//NS_LOG_FUNCTION(this<<'send the ack to'<< fromId);

//build an ACK packet
	int m_ackSize;
	uint8_t *buffer = new uint8_t[m_ackSize = 10];//头部差不多有48个字节，ip头部32个字节，mac头部8个字节，seq头部8个字节
	for (int i = 0; i < m_ackSize; i++) {
		buffer[i] = 19;
	}
	Ptr<Packet> ack = Create<Packet>(buffer, m_ackSize);
	NanoSeqTsHeader seqTs;
	seqTs.SetSeq(1);//here getuid is to get the globalUid, a gloabl parameter

//增加network header
	NanoL3Header header;
	uint32_t src = GetDevice()->GetNode()->GetId();
	uint32_t id = seqTs.GetSeq();
	uint32_t ttl = 0;
	uint32_t qvalue = 100;
	uint32_t hopcount = 1;

	header.SetSource(src);
	header.SetDestination(fromId);
	header.SetTtl(ttl);
	header.SetPacketId(id);
	header.SetQvalue(qvalue);
	header.SetHopCount(hopcount);
	header.SetPrevious(src);
	header.SetQHopCount(0);
	header.SetDeflectRate(0);
	header.SetDropRate(0);
	header.SetEnergyRate(0);
	ack->AddHeader(seqTs);
	ack->AddHeader(header);

	Ptr<NanoMacEntity> mac = GetDevice()->GetMac();
	mac->Send(ack, fromId);
}

void QRouting::SendPacketBuf() {
	NS_LOG_FUNCTION(this);

	while (m_tosendBuffer.front().second < 1) {
		m_tosendBuffer.remove(m_tosendBuffer.front());
	}

	if (m_tosendBuffer.size() >= 1) {
		m_SendingTag = true;
		uint32_t thisId = GetDevice()->GetNode()->GetId();
		thisId = thisId + 1;
		uint32_t nextId = 999; /*= GetDevice()->GetNode()->GetId()*/
		;	//如果找不到就给自己。
		Ptr<Packet> p = (m_tosendBuffer.front().first)->Copy();

		int RepeatCount = m_tosendBuffer.front().second;

		if (RepeatCount < 1) {
			int j = 0;
			j = j + 1;
			//deflect的数据包进行+1操作
			GetDevice()->m_DeflectedCount = GetDevice()->m_DeflectedCount + 1;
		}
		m_tosendBuffer.front().second = RepeatCount - 1;
		//查询相应的macfrom的地址。

		NanoL3Header l3Header;
		p->RemoveHeader(l3Header);
		uint32_t id = l3Header.GetPacketId();



		uint32_t hopcount = l3Header.GetHopCount();
		uint32_t headerQvalue = l3Header.GetQvalue();
		uint32_t qhopcount = l3Header.GetQHopCount();

		//for send the packet in the buffer
		if (hopcount == 1) {
			if (GetDevice()->m_energy < 21) {
				//send
				int energy = GetDevice()->m_energy;
				energy = energy + 1;
				int queue = m_tosendBuffer.size();
				queue = queue + 1;
			} else if (GetDevice()->m_energy >= 21) {
				//for three probabilities
				GetDevice()->ConsumeEnergySendPacket();
				//发送的数据包进行+1操作
				double SendCount = GetDevice()->m_SendCount;
				double DeflectCount = GetDevice()->m_DeflectedCount;
				double ReceiveACKCount = GetDevice()->m_ReceiveACKCount;
				double EnergyLeft = GetDevice()->m_energy;
				double EnergyMax = GetDevice()->m_maxenergy;
				GetDevice()->m_SendCount = SendCount + 1;
				uint32_t deflectrate = DeflectCount / SendCount * 100;
				uint32_t droprate;
				if (SendCount < 1) {
					droprate = 0;
				} else {
					droprate = (SendCount - ReceiveACKCount) / SendCount * 100;
				}
				uint32_t energyrate = (EnergyMax - EnergyLeft) / EnergyMax
						* 100;


				l3Header.SetDeflectRate(deflectrate);
				l3Header.SetDropRate(droprate);
				l3Header.SetEnergyRate(energyrate);
				p->AddHeader(l3Header);
				UpdateSentPacketId(id, nextId);
				Ptr<NanoMacEntity> mac = GetDevice()->GetMac();
				if (GetDevice()->GetNode()->GetId() == nextId) {
					int k = 0;
					k = k + 1;
				}
				//这里需要将路由表置位
				SetRouteUn(nextId);
				mac->Send(p, nextId);

			}
		} else {
//for forward packet
			if (GetDevice()->m_energy < 21) {
				int energy = GetDevice()->m_energy;
				energy = energy + 1;
				int queue = m_tosendBuffer.size();
				queue = queue + 1;
				//下面这行应该注释
				//Simulator::Schedule(Seconds(0.1), &QRouting::ForwardPacket1, this,
				//		p1, macfrom1);
			}
			if (GetDevice()->m_energy >= 21) {
				uint32_t macfrom = SearchMacFrom(id);
				if (macfrom != 9999) {
					int i = 1;
					i = i + 1;
				}
				GetDevice()->ConsumeEnergySendPacket();
				//发送的数据包进行+1操作
				double SendCount = GetDevice()->m_SendCount;
				double DeflectCount = GetDevice()->m_DeflectedCount;
				double ReceiveACKCount = GetDevice()->m_ReceiveACKCount;
				double EnergyLeft = GetDevice()->m_energy;
				double EnergyMax = GetDevice()->m_maxenergy;
				GetDevice()->m_SendCount = SendCount + 1;
				uint32_t deflectrate = DeflectCount / SendCount * 100;
				uint32_t droprate;
				if (SendCount < 1) {
					droprate = 0;
				} else {
					droprate = (SendCount - ReceiveACKCount) / SendCount * 100;
				}
				uint32_t energyrate = (EnergyMax - EnergyLeft) / EnergyMax
						* 100;


				l3Header.SetPrevious(GetDevice()->GetNode()->GetId());
				l3Header.SetQvalue(headerQvalue);
				l3Header.SetQHopCount(qhopcount);
				l3Header.SetDeflectRate(deflectrate);
				l3Header.SetDropRate(droprate);
				l3Header.SetEnergyRate(energyrate);
				NS_LOG_FUNCTION(this<<"forward new l3 header"<<l3Header);
				p->AddHeader(l3Header);

				UpdateSentPacketId(id, nextId);
				Ptr<NanoMacEntity> mac = GetDevice()->GetMac();
				if (GetDevice()->GetNode()->GetId() == nextId) {
					int k = 0;
					k = k + 1;
				}
				//这里将路由表置位
				SetRouteUn(nextId);
				mac->Send(p, nextId);
			}
		}

		Simulator::Schedule(PicoSeconds(200000), &QRouting::SendPacketBuf,
				this);
	} else {
		m_SendingTag = false;
	}
}

void QRouting::ForwardPacket(Ptr<Packet> p) {
}

void QRouting::ForwardPacket1(Ptr<Packet> p, uint32_t macfrom) {
	NS_LOG_FUNCTION(this);


	Ptr<Packet> p1 = p;

	NanoL3Header l3Header;
	p->RemoveHeader(l3Header);
	uint32_t id = l3Header.GetPacketId();
	NS_LOG_FUNCTION(this<<l3Header);
	NS_LOG_FUNCTION(this<<"packet id"<< id<< "from"<<macfrom);



	uint32_t headerQvalue = l3Header.GetQvalue();	//
	uint32_t hopcount = l3Header.GetHopCount();	//
	uint32_t ttl = l3Header.GetTtl();
	uint32_t qhopcount = l3Header.GetQHopCount() + 1;

	/*	if (deflectedId != 995) {
	 routenextId = LookupRoute(to);
	 }*/
	uint32_t thisid = GetDevice()->GetNode()->GetId();

//uint32_t macfrom1 = macfrom;

	if (ttl > 1) {
		if (GetDevice()->m_energy < 21
				|| m_tosendBuffer.size() >= GetDevice()->m_bufferSize) {
			int energy = GetDevice()->m_energy;
			energy = energy + 1;
			int queue = m_tosendBuffer.size();
			queue = queue + 1;
			//下面这行应该注释
			//Simulator::Schedule(Seconds(0.1), &QRouting::ForwardPacket1, this,
			//		p1, macfrom1);
		} else if (GetDevice()->m_energy >= 21
				&& m_tosendBuffer.size() < GetDevice()->m_bufferSize) {
			int energy = GetDevice()->m_energy;
			energy = energy + 1;


			l3Header.SetTtl(ttl - 1);
			l3Header.SetHopCount(hopcount + 1);
			l3Header.SetPrevious(thisid);
			l3Header.SetQvalue(headerQvalue);
			l3Header.SetQHopCount(qhopcount);		//将搜索到的路由表中的跳数用于q值的更新。
			NS_LOG_FUNCTION(this<<"forward new l3 header"<<l3Header);
			p->AddHeader(l3Header);

			//收到包，进行压栈
			//GetDevice()->m_queuePacket.push_back(p);
			if (m_SendingTag == true) {
				UpdateToSendBuffer(p);
				UpdateMacFrom(id, macfrom);
			} else {
				UpdateToSendBuffer(p);
				UpdateMacFrom(id, macfrom);
				SendPacketBuf();
			}
		}
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

void QRouting::UpdateToSendBuffer(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this);

	std::pair<Ptr<Packet>, int> item;
	item.first = p;
	item.second = 1;
	m_tosendBuffer.push_back(item);
}

void QRouting::UpdateMacFrom(uint32_t id, uint32_t macfrom) {
	NS_LOG_FUNCTION(this);
	std::pair<uint32_t, uint32_t> item;
	item.first = id;
	item.second = macfrom;
	m_macFrom.push_back(item);
}

uint32_t QRouting::SearchMacFrom(uint32_t id) {
	NS_LOG_FUNCTION(this);
	std::list<std::pair<uint32_t, uint32_t>>::iterator it = m_macFrom.begin();

	uint16_t i = 0;
	for (; it != m_macFrom.end(); ++it) {
		if ((*it).first == id) {
			break;
		}
		i++;
	}
	if (i < m_macFrom.size()) {
		return (*it).second;
	} else {
		return 9999;
	}
}

}
