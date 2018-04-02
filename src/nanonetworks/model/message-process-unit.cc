/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013,2014 TELEMATICS LAB, DEI - Politecnico di Bari
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Giuseppe Piro <peppe@giuseppepiro.com>, <g.piro@poliba.it>
 */

#include "message-process-unit.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/packet.h"
#include "simple-nano-device.h"
#include "nano-mac-queue.h"
#include "nano-spectrum-phy.h"
#include "nano-mac-header.h"
#include "nano-seq-ts-header.h"
#include "ns3/simulator.h"
#include "ns3/nano-l3-header.h"

NS_LOG_COMPONENT_DEFINE("MessageProcessUnit");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(MessageProcessUnit);

TypeId MessageProcessUnit::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::MessageProcessUnit").SetParent<Object>().AddTraceSource(
					"outTX", "outTX",
					MakeTraceSourceAccessor(&MessageProcessUnit::m_outTX),
					"ns3::MessageProcessUnit::OutTxCallback").AddTraceSource(
					"outRX", "outRX",
					MakeTraceSourceAccessor(&MessageProcessUnit::m_outRX),
					"ns3::MessageProcessUnit::OutRxCallback");
	;
	return tid;
}

MessageProcessUnit::MessageProcessUnit() {
	NS_LOG_FUNCTION(this);
	m_device = 0;
	m_packetSize = 0;
	m_interarrivalTime = 99999999999;
}

MessageProcessUnit::~MessageProcessUnit() {
	NS_LOG_FUNCTION(this);
}

void MessageProcessUnit::DoDispose(void) {
	NS_LOG_FUNCTION(this);
	m_device = 0;
}

void MessageProcessUnit::SetDevice(Ptr<SimpleNanoDevice> d) {
	NS_LOG_FUNCTION(this);
	m_device = d;
}

Ptr<SimpleNanoDevice> MessageProcessUnit::GetDevice(void) {
	return m_device;
}

void MessageProcessUnit::CreteMessage() {
	NS_LOG_FUNCTION(this);

	if (m_device->m_energy < 21 && m_device->m_queuePacket.size() >= 1) {

		int energy = GetDevice()->m_energy;
		energy = energy + 1;
	} else if (m_device->m_energy >= 21 && m_device->m_queuePacket.size() < m_device->m_bufferSize) {
		uint8_t *buffer = new uint8_t[m_packetSize = 102];
		for (int i = 0; i < m_packetSize; i++) {
			buffer[i] = 129;
		}
		Ptr<Packet> p = Create<Packet>(buffer, m_packetSize);
		NanoSeqTsHeader seqTs;
		seqTs.SetSeq(p->GetUid());
		p->AddHeader(seqTs);

		srand(m_randv);
		m_randv = m_randv + 50;
		m_dstId = (rand() % (29 - 0 + 1)) + 0;

		m_outTX((int) p->GetUid(), (int) GetDevice()->GetNode()->GetId(),
				(int) m_dstId);

		m_device->SendPacketDst(p, m_dstId);
		Simulator::Schedule(Seconds(m_interarrivalTime),
				&MessageProcessUnit::CreteMessage, this);
	}
}

void MessageProcessUnit::ProcessMessage(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this);

	NanoL3Header l3Header;
	p->RemoveHeader(l3Header);

	NanoSeqTsHeader seqTs;
	p->RemoveHeader(seqTs);

	NS_LOG_FUNCTION(this << l3Header);
	NS_LOG_FUNCTION(this << seqTs);

	double delay = Simulator::Now().GetPicoSeconds()
			- seqTs.GetTs().GetPicoSeconds();

	m_outRX(seqTs.GetSeq(), p->GetSize(), (int) l3Header.GetSource(),
			(int) GetDevice()->GetNode()->GetId(), delay,
			(int) l3Header.GetTtl(), (int) l3Header.GetHopCount(),
			(int) l3Header.GetQHopCount());
}

void MessageProcessUnit::SetPacketSize(int s) {
	NS_LOG_FUNCTION(this);
	m_packetSize = s;
}

void MessageProcessUnit::SetInterarrivalTime(double t) {
	NS_LOG_FUNCTION(this);
	m_interarrivalTime = t;
}
void MessageProcessUnit::SetDstId(uint32_t dstId) {
	NS_LOG_FUNCTION(this);
	m_randv = dstId;
}

} // namespace ns3
