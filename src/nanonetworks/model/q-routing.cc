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

	/*//能量限制，每次要发送或者接受都需要判断一下能量是否足够等。
	 SendPacketRecACK = GetDevice()->m_EnergySendPerByte
	 * GetDevice()->m_PacketSize
	 + GetDevice()->m_EnergyReceivePerByte * GetDevice()->m_ACKSize;
	 RecACK = GetDevice()->m_EnergyReceivePerByte * GetDevice()->m_ACKSize;
	 RecPacketForward = GetDevice()->m_EnergySendPerByte
	 * GetDevice()->m_PacketSize
	 + GetDevice()->m_EnergyReceivePerByte * GetDevice()->m_ACKSize
	 + GetDevice()->m_EnergySendPerByte * GetDevice()->m_ACKSize
	 + GetDevice()->m_EnergySendPerByte * GetDevice()->m_PacketSize;*/
}

QRouting::~QRouting() {
	SetDevice(0);
}

void QRouting::DoDispose() {
	SetDevice(0);
}

void QRouting::AddRoute(uint32_t dstId, uint32_t nextId, uint32_t Qvalue,
		uint32_t HopCount, double energyharvestspeed, double energyconsumespeed,
		double energyconsumerate) {
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
	Qroute.RouteValid = true;
	//for energy prediction
	Qroute.energyharvestspeed = energyharvestspeed;
	Qroute.energyconsumespeed = energyconsumespeed;
	Qroute.energyconsumerate = energyconsumerate;

	m_qtable.push_back(Qroute);
}

void QRouting::AddPreNodeNew(uint32_t dstId, uint32_t nextId, uint32_t Qvalue,
		uint32_t HopCount, double energyharvestspeed, double energyconsumespeed,
		double energyconsumerate) {
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
	//for energy prediction
	PreNode.energyharvestspeed = energyharvestspeed;
	PreNode.energyconsumespeed = energyconsumespeed;
	PreNode.energyconsumerate = energyconsumerate;
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
	Simulator::Schedule(Seconds(3), &QRouting::SetRouteAv, this, nextId);
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

	PreNodeEntry cPreNode;
	//prediction, 预测还有能量才选择。
	double energystatus_pre_c;
	double energy_min_send = GetDevice()->m_EnergySendPerByte
			* GetDevice()->m_PacketSize
			+ GetDevice()->m_EnergyReceivePerByte * GetDevice()->m_ACKSize;
	double deltaT = Simulator::Now().GetSeconds();

	for (; it != m_prenode.end(); ++it) {
		if (it->dstId == dstId && it->nodeId != routeNextId
				&& it->PreNodeValid == true) {

			energystatus_pre_c = (1 - it->energyconsumerate)
					* GetDevice()->m_maxenergy
					+ (it->energyharvestspeed - it->energyconsumespeed)
							* (Simulator::Now().GetSeconds() - it->UpTime);
			//小于最小发射能量就不选择了。
			if (energystatus_pre_c >= energy_min_send) {
				j = i;
				break;
			}
		}
		i++;
	}
	cPreNode = m_prenode[j];

	//迭代寻找最有的节点。
	double energyconsumerate_pre_it;
	double energyconsumerate_pre_c = (cPreNode.energyconsumerate
			* GetDevice()->m_maxenergy
			+ (cPreNode.energyharvestspeed - cPreNode.energyconsumespeed)
					* (Simulator::Now().GetSeconds() - cPreNode.UpTime))
			/ GetDevice()->m_maxenergy;
	for (int i = 0; it != m_prenode.end(); ++it) {
		if (it->dstId == dstId && it->nodeId != routeNextId
				&& it->PreNodeValid == true) {

			energyconsumerate_pre_it = (it->energyconsumerate
					* GetDevice()->m_maxenergy
					+ (it->energyharvestspeed - it->energyconsumespeed)
							* (Simulator::Now().GetSeconds() - it->UpTime))
					/ GetDevice()->m_maxenergy;

			if ((it->Qvalue + (deltaT - it->UpTime) * it->RecRate)
					* energyconsumerate_pre_it
					< (cPreNode.Qvalue
							+ (deltaT - cPreNode.UpTime) * cPreNode.RecRate)
							* energyconsumerate_pre_c) { // 寻找最小值
				j = i;
				cPreNode = m_prenode[j];
				energyconsumerate_pre_c = (cPreNode.energyconsumerate
						* GetDevice()->m_maxenergy
						+ (cPreNode.energyharvestspeed
								- cPreNode.energyconsumespeed)
								* (Simulator::Now().GetSeconds()
										- cPreNode.UpTime))
						/ GetDevice()->m_maxenergy;
			}
		}
		i++;
	}

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
		uint32_t HopCount, double energyharvestspeed, double energyconsumespeed,
		double energyconsumerate) {
	std::vector<QTableEntry>::iterator it = m_qtable.begin();

	uint16_t i = 0;
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
		Qroute.energyharvestspeed = energyharvestspeed;
		Qroute.energyconsumespeed = energyconsumespeed;
		Qroute.energyconsumerate = energyconsumerate;
		return Qroute.Qvalue;
	} else {
		return 996;
	}
}

void QRouting::UpdatePreNode(uint32_t dstId, uint32_t nextId, double reward,
		uint32_t HopCount, double energyharvestspeed, double energyconsumespeed,
		double energyconsumerate) {
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
		//for energy prediction
		PreNode.energyharvestspeed = energyharvestspeed;
		PreNode.energyconsumespeed = energyconsumespeed;
		PreNode.energyconsumerate = energyconsumerate;
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

	if (GetDevice()->m_energy
			< GetDevice()->m_EnergySendPerByte * GetDevice()->m_PacketSize
					+ GetDevice()->m_EnergyReceivePerByte
							* GetDevice()->m_ACKSize
			|| m_tosendBuffer.size() >= GetDevice()->m_bufferSize) {
		//send
		int energy = GetDevice()->m_energy;
		energy = energy + 1;
		int queue = m_tosendBuffer.size();
		queue = queue + 1;
	} else if (GetDevice()->m_energy
			>= GetDevice()->m_EnergySendPerByte * GetDevice()->m_PacketSize
					+ GetDevice()->m_EnergyReceivePerByte
							* GetDevice()->m_ACKSize
			&& m_tosendBuffer.size() < GetDevice()->m_bufferSize) {
		NanoSeqTsHeader seqTs;
		p->RemoveHeader(seqTs);
		NS_LOG_FUNCTION(this<<p<<"size"<<p->GetSize()<<seqTs);

		uint32_t dst = dstNodeId; //目的地址
		//uint32_t nextId = 999; //GetDevice()->GetNode()->GetId(); //如果找不到就给自己。

		//确定头部的内容
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

	if (macto == GetDevice()->GetNode()->GetId()) {
		NS_LOG_FUNCTION(this<<"is for me");
		//GetDevice()->GetMessageProcessUnit()->ProcessMessage(p);

		if (p->GetSize() < 102)	//因为一个正常的包的大小是102bytes，所以小于这个大小的时候就是ACK,为什么取102呢？其实是瞎选的，但是保证packet的个数一定大于这个，ACK的大小一定小于这个。
				{
			if (GetDevice()->m_energy
					< GetDevice()->m_EnergyReceivePerByte
							* GetDevice()->m_ACKSize) {
				int energy = GetDevice()->m_energy;
				energy = energy + 1;
			} else {
				//接收的ACK进行+1的操作
				GetDevice()->m_ReceiveACKCount = GetDevice()->m_ReceiveACKCount
						+ 1;
				m_tosendBuffer.remove(m_tosendBuffer.front());
				//consume energy
				//GetDevice()->ConsumeEnergyRecACK();
				GetDevice()->ConsumeEnergyReceive(GetDevice()->m_ACKSize);
				//NS_LOG_FUNCTION(this<<'receive the ack'<<p<<'size'<<p->GetSize());
				SetRouteAv(macfrom);
			}
		} else {
			if (GetDevice()->m_energy
					<= GetDevice()->m_EnergySendPerByte
							* GetDevice()->m_PacketSize
							+ GetDevice()->m_EnergyReceivePerByte
									* GetDevice()->m_ACKSize
							+ GetDevice()->m_EnergySendPerByte
									* GetDevice()->m_ACKSize
							+ GetDevice()->m_EnergySendPerByte
									* GetDevice()->m_PacketSize
					|| m_tosendBuffer.size() >= GetDevice()->m_bufferSize) {
				//receive
				int energy = GetDevice()->m_energy;
				energy = energy + 1;
				int queue = m_tosendBuffer.size();
				queue = queue + 1;
			}
			if (GetDevice()->m_energy
					> GetDevice()->m_EnergySendPerByte
							* GetDevice()->m_PacketSize
							+ GetDevice()->m_EnergyReceivePerByte
									* GetDevice()->m_ACKSize
							+ GetDevice()->m_EnergySendPerByte
									* GetDevice()->m_ACKSize
							+ GetDevice()->m_EnergySendPerByte
									* GetDevice()->m_PacketSize
					&& m_tosendBuffer.size() < GetDevice()->m_bufferSize) {
				//GetDevice()->ConsumeEnergyRecPacket();
				GetDevice()->ConsumeEnergyReceive(GetDevice()->m_PacketSize);

				//GetDevice()->ConsumeEnergySendACK();
				GetDevice()->ConsumeEnergySend(GetDevice()->m_ACKSize);

				//接收的数据包进行+1操作
				GetDevice()->m_ReceiveCount = GetDevice()->m_ReceiveCount + 1;
				NanoL3Header l3Header;
				p->RemoveHeader(l3Header);	//这里删除了，后面就需要进行增加。

				uint32_t from = l3Header.GetSource();
				uint32_t to = l3Header.GetDestination();
				uint32_t previous = l3Header.GetPrevious();
				uint32_t qvalue = l3Header.GetQvalue();
				uint32_t hopcount = l3Header.GetHopCount();
				uint32_t qhopcount = l3Header.GetQHopCount();

				double deflectrate = l3Header.GetDeflectRate();
				double droprate = l3Header.GetDropRate();
				double energyrate = l3Header.GetEnergyRate();

				//for energy prediction
				double energyharvestspeed = l3Header.GetEnergyHarvestSum()
						/ 100000.0 / Simulator::Now().GetSeconds();
				//energyharvestsum = energyharvestsum + 1;
				double energyconsumespeed = l3Header.GetEnergyConsumeSum()
						/ 100000.0 / Simulator::Now().GetSeconds();
				//energyconsumesum = energyconsumesum + 1;
				double time = Simulator::Now().GetSeconds();
				time = time + 1;
				uint32_t thisid = GetDevice()->GetNode()->GetId();
				//下面这个函数打印节点的状态，在收到一个数据包后进行打印
				GetDevice()->GetMessageProcessUnit()->PrintNodestatus();
				//！！！！！update the q routing table and prenode table
				bool PreNodeE = SearchPreNode(from, previous);
				if (hopcount == 1) {
					if (!PreNodeE && previous != thisid) {
						//add the q value table, i.e. the prenode
						AddPreNodeNew(from, previous, qvalue, hopcount,
								energyharvestspeed, energyconsumespeed,
								energyrate);
						uint32_t route = LookupRoute(from);
						if (route == 991 && previous != thisid) {//路由表中已经有记录了，就不行进行增加了
							AddRoute(from, previous, qvalue, hopcount,
									energyharvestspeed, energyconsumespeed,
									energyrate);
						}
						//AddRoute(from, previous, qvalue, hopcount);
					} else {
						//也就是存在一跳的邻居节点。也就不用更新了，因为只有一跳。
					}
				} else {		//跳数大于一跳
					if (!PreNodeE && previous != thisid) {
						AddPreNodeNew(from, previous, qvalue, hopcount,
								energyharvestspeed, energyconsumespeed,
								energyrate);
						uint32_t route = LookupRoute(from);
						if (route == 991 && previous != thisid) {//路由表中已经有记录了，就不行进行增加了
							AddRoute(from, previous, qvalue, hopcount,
									energyharvestspeed, energyconsumespeed,
									energyrate);
						}
					} else if (PreNodeE && previous != thisid) {
						//
						//对routingtable 进行更新
						//energyrate是指消耗的能量的多少
						//这里的1+。。。感觉应该对原来论文中的公式进行修改。或者去掉这里的1+？？？感觉还是更偏向于前者。
						double preReward = qvalue * (qhopcount + 1)
								* (1 + deflectrate) * (1 + droprate)
								* (1 + energyrate) / 1000000;
						UpdatePreNode(from, previous, preReward, qhopcount,
								energyharvestspeed, energyconsumespeed,
								energyrate);
						uint32_t newQvalue = UpdateQvalue(from, preReward,
								qhopcount, energyharvestspeed,
								energyconsumespeed, energyrate);	//更新路由表上的Q值
						if (newQvalue == 996) {
						} else {
							UpdateRoute(from, previous, newQvalue, qhopcount);
						}
					}
				}
				if (to == GetDevice()->GetNode()->GetId()) {
					//如果节点是自己本身，那么只要返回简单的ACK就可以了。
					SendACKPacket(macfrom);	//这个ACK的内容没有办法更新的。
					NS_LOG_FUNCTION(this<<"i am the destination");
					p->AddHeader(l3Header);
					GetDevice()->GetMessageProcessUnit()->ProcessMessage(p);
				} else {
					p->AddHeader(l3Header);
					NS_LOG_FUNCTION(this<<"forward!");
					ForwardPacket1(p, macfrom);
				}
			}
		}
	}
}

void QRouting::SendACKPacket(uint32_t fromId) {
//NS_LOG_FUNCTION(this<<'send the ack to'<< fromId);

//build an ACK packet
	int m_ackSize;
	uint8_t *buffer = new uint8_t[m_ackSize = 10];//小于70的是普通ack，小于102的是feedback的ack。头部差不多有48个字节，ip头部32个字节，mac头部8个字节，seq头部8个字节
	for (int i = 0; i < m_ackSize; i++) {
		buffer[i] = 10;
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
	//下面的数据是有用的，在feedback的ack中是用来feedback的更新。

	header.SetQvalue(qvalue);
	header.SetHopCount(hopcount);
	header.SetPrevious(src);
	header.SetQHopCount(0);
	header.SetDeflectRate(0);
	header.SetDropRate(0);
	header.SetEnergyRate(0);
	//上面的准确来说是用来更新q、Route、deflect table的。
// for the energy prediction
	uint32_t energyharvestsum = GetDevice()->m_energyHarvestSum * 100000.0;
	uint32_t energyconsumesum = GetDevice()->m_energyConsumeSum * 100000.0;
	header.SetEnergyHarvestSum(energyharvestsum);
	header.SetEnergyConsumeSum(energyconsumesum);
	ack->AddHeader(seqTs);
	ack->AddHeader(header);
	Ptr<NanoMacEntity> mac = GetDevice()->GetMac();
	mac->Send(ack, fromId);
}

//feedback的
void QRouting::SendACKFeedback(uint32_t destination, uint32_t macfrom,
		uint32_t qvalue, uint32_t hopcount, uint32_t nextid,
		uint32_t deflectrate, uint32_t droprate, uint32_t energyrate) {
	//build the ack feedback packet
	int m_acksize;
	uint8_t *buffer = new uint8_t[m_acksize = 50];//小于70的是普通ack，小于102的是feedback的ack。这里要注意跟普通ACK的区别，具体的区分，可以通过packet的大小计算一共是多少。
	for (int i = 0; i < m_acksize; i++) {
		buffer[i] = 50;
	}
	Ptr<Packet> ack = Create<Packet>(buffer, m_acksize);
	NanoSeqTsHeader seqTs;
	seqTs.SetSeq(1);	//这个我也忘记了，回头好好思考一下，现在就按照原来的写吧。06/01/2018
	//here getuid is to get the globaluid, a global parameter

	//增加network header，用于ack的包头。
	NanoL3Header header;
	uint32_t src = GetDevice()->GetNode()->GetId();
	uint32_t id = seqTs.GetSeq();
	uint32_t ttl = 0;

	header.SetSource(src);
	header.SetDestination(macfrom);
	header.SetTtl(ttl);
	header.SetPacketId(id);

	//下面的数据是有用的，用来feedback的更新。
	//准确来说是用来更新q、Route、deflect table的。
	header.SetQvalue(qvalue);
	header.SetHopCount(hopcount);	//这个是包真的跳过的跳数。
	header.SetPrevious(nextid);
	header.SetQHopCount(hopcount);//这里在feedforward中其实是路由表或者偏转表上查到的，但是不是包真的经过的跳数。
	header.SetDeflectRate(deflectrate);
	header.SetDropRate(droprate);
	header.SetEnergyRate(energyrate);
	// for the energy prediction
	uint32_t energyharvestsum = GetDevice()->m_energyHarvestSum * 100000.0;
	uint32_t energyconsumesum = GetDevice()->m_energyConsumeSum * 100000.0;
	header.SetEnergyHarvestSum(energyharvestsum);
	header.SetEnergyConsumeSum(energyconsumesum);
	ack->AddHeader(seqTs);
	ack->AddHeader(header);
	Ptr<NanoMacEntity> mac = GetDevice()->GetMac();
	mac->Send(ack, macfrom);
}

void QRouting::ForwardPacket(Ptr<Packet> p) {
}

void QRouting::ForwardPacket1(Ptr<Packet> p, uint32_t macfrom) {
	NS_LOG_FUNCTION(this);

//uint32_t nextId = 99;		// GetDevice()->GetNode()->GetId();	//如果找不到就给自己。
	Ptr<Packet> p1 = p;

	NanoL3Header l3Header;
	p->RemoveHeader(l3Header);
	uint32_t id = l3Header.GetPacketId();
	NS_LOG_FUNCTION(this<<l3Header);
	NS_LOG_FUNCTION(this<<"packet id"<< id<< "from"<<macfrom);

//uint32_t from = l3Header.GetSource();
//uint32_t to = l3Header.GetDestination();
	uint32_t headerQvalue = l3Header.GetQvalue();	//
	uint32_t hopcount = l3Header.GetHopCount();	//
	uint32_t ttl = l3Header.GetTtl();
	uint32_t qhopcount = l3Header.GetQHopCount() + 1;
	uint32_t thisid = GetDevice()->GetNode()->GetId();

	if (ttl > 1) {
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

	} else {
		NS_LOG_FUNCTION(this<<"ttl expired");
	}
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
			//在buffersize=1的情况下，根本一直没有加；所以要改一下位置。

		}
		m_tosendBuffer.front().second = RepeatCount - 1;

		NanoL3Header l3Header;
		p->RemoveHeader(l3Header);
		uint32_t id = l3Header.GetPacketId();

		uint32_t from = l3Header.GetSource();
		uint32_t to = l3Header.GetDestination();
		uint32_t hopcount = l3Header.GetHopCount();
		uint32_t headerQvalue = l3Header.GetQvalue();
		uint32_t qhopcount = l3Header.GetQHopCount();

		//查询相应的macfrom的地址。这样nodes就能知道将ACK发送给谁。
		uint32_t macfrom = SearchMacFrom(id);

		//for send the packet in the buffer
		if (hopcount == 1) {
			if (GetDevice()->m_energy
					< GetDevice()->m_EnergySendPerByte
							* GetDevice()->m_PacketSize
							+ GetDevice()->m_EnergyReceivePerByte
									* GetDevice()->m_ACKSize) {
				//send
				int energy = GetDevice()->m_energy;
				energy = energy + 1;
				int queue = m_tosendBuffer.size();
				queue = queue + 1;
			} else if (GetDevice()->m_energy
					>= GetDevice()->m_EnergySendPerByte
							* GetDevice()->m_PacketSize
							+ GetDevice()->m_EnergyReceivePerByte
									* GetDevice()->m_ACKSize) {
				//for three probabilities
				//GetDevice()->ConsumeEnergySendPacket();
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

				uint32_t routenextId = LookupRoute(to);
				bool routeava = RouteAvailable(to);
				uint32_t deflectId;
				if (routenextId != 991) {
					deflectId = ChooseDeflectNode(to, routenextId);
				}

				if (routenextId != 991 && routeava) { //如果目的地址不是邻居节点，但是在路由表上
					//获取邻居节点号  路由表发送
					nextId = LookupRoute(to);
					NS_LOG_FUNCTION(
							this<<"There is a route" << "nextId" << nextId);
					//在发出去之前发送可以更新路由表的ack到上一个节点？
					uint32_t qvalue = SearchRouteForQvalue(to);
					SendACKFeedback(to, macfrom, qvalue, hopcount, nextId,
							deflectrate, droprate, energyrate);

				} else if (routenextId != 991 && !routeava
						&& deflectId != 995) {
					nextId = deflectId;
					//在发出去之前发送可以更新路由表的ack到上一个节点？
					uint32_t qvalue = SearchRouteForQvalue(to);
					SendACKFeedback(to, macfrom, qvalue, hopcount, nextId,
							deflectrate, droprate, energyrate);

					//既然nextId选择了deflect的形式，那么+1是很合理的。
					GetDevice()->m_DeflectedCount =
							GetDevice()->m_DeflectedCount + 1;
					NS_LOG_FUNCTION(
							this<<"The packet is deflected"<<"nextId"<<nextId);
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
						//如果是随机选择的节点，那么发回的ack不能用于更新路由表,只要发个ACK回去就行了。
						SendACKPacket(macfrom);
					}
					NS_LOG_FUNCTION(
							this<<"random choose a neighbor" << "nextId" << nextId);
				}
				l3Header.SetDeflectRate(deflectrate);
				l3Header.SetDropRate(droprate);
				l3Header.SetEnergyRate(energyrate);
				// for the energy prediction
				uint32_t energyharvestsum = GetDevice()->m_energyHarvestSum
						* 100000.0;
				uint32_t energyconsumesum = GetDevice()->m_energyConsumeSum
						* 100000.0;
				l3Header.SetEnergyHarvestSum(energyharvestsum);
				l3Header.SetEnergyConsumeSum(energyconsumesum);
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
				//消耗的能量，本来应该放在真正发送的时候，但是为了和接受保持一致，所以放在这里了。
				//所以，168其中包括数据128,40头部。这里把mac和route head都算上去了。
				GetDevice()->ConsumeEnergySend(GetDevice()->m_PacketSize);
			}
		} else {
//for forward packet
			if (GetDevice()->m_energy
					< GetDevice()->m_EnergySendPerByte
							* GetDevice()->m_PacketSize
							+ GetDevice()->m_EnergyReceivePerByte
									* GetDevice()->m_ACKSize) {
				//下面四行代码是没用的，就是看看中间值
				int energy = GetDevice()->m_energy;
				energy = energy + 1;
				int queue = m_tosendBuffer.size();
				queue = queue + 1;
				//下面这行应该注释
				//Simulator::Schedule(Seconds(0.1), &QRouting::ForwardPacket1, this,
				//		p1, macfrom1);
			}
			if (GetDevice()->m_energy
					>= GetDevice()->m_EnergySendPerByte
							* GetDevice()->m_PacketSize
							+ GetDevice()->m_EnergyReceivePerByte
									* GetDevice()->m_ACKSize) {
				uint32_t macfrom = SearchMacFrom(id);
				//下面的代码也是没用的，就是看看中间值
				if (macfrom != 9999) {
					int i = 1;
					i = i + 1;
				}
				//GetDevice()->ConsumeEnergySendPacket();
				//发送的数据包进行+1操作
				double SendCount = GetDevice()->m_SendCount;
				double DeflectCount = GetDevice()->m_DeflectedCount;
				double ReceiveACKCount = GetDevice()->m_ReceiveACKCount;
				//下面这个是没有用的，就是看一下中间值。
				if (DeflectCount != 0) {
					int i = 0;
					i = i + 1;
				}
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

				uint32_t routenextId = LookupRoute(to);
				uint32_t deflectedId = 995;
				bool PalreadySent;				//判断是否已经偏转过这个节点了。
				bool TalreadySent;				//判断是否已经发送过这个节点了。
				if (routenextId != 991) {
					TalreadySent = CheckAmongSentPacket(id, routenextId);
				}
				bool routeava = RouteAvailable(to);
				if (routenextId != 991 && !routeava) {
					deflectedId = ChooseDeflectNode(to, routenextId);
					if (deflectedId != 995) {
						PalreadySent = CheckAmongSentPacket(id, deflectedId);
					}
				}

				if (routenextId != 991 && routeava && !TalreadySent) {
					nextId = routenextId;
					//如果是根据路由表选择出来的下一跳，那么就需要进行feedback的更新
					uint32_t qvalue = SearchRouteForQvalue(to);
					SendACKFeedback(to, macfrom, qvalue, hopcount, nextId,
							deflectrate, droprate, energyrate);

					NS_LOG_FUNCTION(
							this<<"Forward there is route"<<"nextId"<<nextId);
					// 获得 from ，previous 对应的qvalue 和hopcount；
					headerQvalue = SearchRouteForQvalue(from);
					qhopcount = SearchRouteForQHopCount(from);
				} else if (routenextId != 991 && !routeava && deflectedId != 995
						&& !PalreadySent) {
					nextId = deflectedId;
					//如果是从偏转表中选择出来的下一跳，那么就需要进行feedback的更新。
					uint32_t qvalue = SearchRouteForQvalue(to);
					SendACKFeedback(to, macfrom, qvalue, hopcount, nextId,
							deflectrate, droprate, energyrate);

					//既然nextId选择了deflect的形式，那么+1是很合理的。
					GetDevice()->m_DeflectedCount =
							GetDevice()->m_DeflectedCount + 1;
					NS_LOG_FUNCTION(
							this<<"Forward the packet is deflected"<<"nextId"<<nextId);
					// 获得 from ，previous 对应的qvalue 和hopcount；
					headerQvalue = SearchRouteForQvalue(from);
					qhopcount = SearchRouteForQHopCount(from);
				} else {
					std::vector<std::pair<uint32_t, uint32_t>> newNeighbors;
					std::vector<std::pair<uint32_t, uint32_t>> neighbors =
							GetDevice()->GetMac()->m_neighbors;
					std::vector<std::pair<uint32_t, uint32_t>>::iterator it =
							neighbors.begin();
					for (; it != neighbors.end(); ++it) {
						uint32_t nextId = (*it).first;
						bool NalreadySent = CheckAmongSentPacket(id, nextId);
						if (!NalreadySent) {
							NS_LOG_FUNCTION(this<<"Consider these nodes");
							newNeighbors.push_back(*it);
						}
					}
					NS_LOG_FUNCTION(
							this<<"newNeighbors.size()" << newNeighbors.size());
					if (newNeighbors.size() > 1) {
						nextId = macfrom;
						while (nextId == macfrom) {
							srand(time(NULL));
							int i = rand() % newNeighbors.size();
							nextId = neighbors[i].first;
						}
						//如果是通过随机方法选择的下一跳，那么只需要简单ACK返回就可以了。
						SendACKPacket(macfrom);

					} else if (newNeighbors.size() == 1
							&& newNeighbors.at(0).first != macfrom) {
						NS_LOG_FUNCTION(
								this << "select the only available neighbors");
						nextId = newNeighbors.at(0).first;
						//如果是通过随机方法选择的下一跳，那么只需要简单ACK返回就可以了。
						SendACKPacket(macfrom);

					} else if (newNeighbors.size() == 1
							&& newNeighbors.at(0).first == macfrom) {
						NS_LOG_FUNCTION(
								this << "select the only available neighbors");
						nextId = newNeighbors.at(0).first;
						//如果是通过随机方法选择的下一跳，那么只需要简单ACK返回就可以了。
						SendACKPacket(macfrom);

					}
					NS_LOG_FUNCTION(
							this<<"random choose a neighbor"<<"nextId"<<nextId);
				}
				l3Header.SetPrevious(GetDevice()->GetNode()->GetId());
				l3Header.SetQvalue(headerQvalue);
				l3Header.SetQHopCount(qhopcount);
				l3Header.SetDeflectRate(deflectrate);
				l3Header.SetDropRate(droprate);
				l3Header.SetEnergyRate(energyrate);
				// for the energy prediction
				uint32_t energyharvestsum = GetDevice()->m_energyHarvestSum
						* 100000.0;
				uint32_t energyconsumesum = GetDevice()->m_energyConsumeSum
						* 100000.0;
				l3Header.SetEnergyHarvestSum(energyharvestsum);
				l3Header.SetEnergyConsumeSum(energyconsumesum);
				NS_LOG_FUNCTION(this<<"forward new l3 header"<<l3Header);
				p->AddHeader(l3Header);

				UpdateSentPacketId(id, nextId);
				Ptr<NanoMacEntity> mac = GetDevice()->GetMac();
				if (GetDevice()->GetNode()->GetId() == nextId) {
					int k = 0;
					k = k + 1;
				}
				//在发出去之前发送可以更新路由表的ack到上一个节点？

				//这里将路由表置位
				SetRouteUn(nextId);
				mac->Send(p, nextId);
				//转发数据包消耗的能量。
				GetDevice()->ConsumeEnergySend(GetDevice()->m_PacketSize);
			}
		}
//重新调用发送缓存区的函数，需要有延时，这里需要进行改进。
		Simulator::Schedule(PicoSeconds(200000), &QRouting::SendPacketBuf,
				this);
	} else {
		m_SendingTag = false;
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
