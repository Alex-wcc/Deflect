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

#include "ns3/log.h"
#include "nano-l3-header.h"

NS_LOG_COMPONENT_DEFINE("NanoL3Header");

namespace ns3 {

TypeId NanoL3Header::GetTypeId(void) {
	static TypeId tid =
			TypeId("NanoL3Header").SetParent<Header>().AddConstructor<
					NanoL3Header>();
	return tid;
}

TypeId NanoL3Header::GetInstanceTypeId(void) const {
	return GetTypeId();
}

uint32_t NanoL3Header::GetSerializedSize(void) const {
	return 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4;
}

void NanoL3Header::Serialize(Buffer::Iterator start) const {
	start.WriteHtonU32(m_source);
	start.WriteHtonU32(m_destination);
	start.WriteHtonU32(m_ttl);
	start.WriteHtonU32(m_packetId);
	start.WriteHtonU32(m_intqvalue);
	start.WriteHtonU32(m_hopcount);
	start.WriteHtonU32(m_previous);
	start.WriteHtonU32(m_qhopcount);
	//for three probabilities
	start.WriteHtonU32(m_deflectrate);
	start.WriteHtonU32(m_droprate);
	start.WriteHtonU32(m_energyrate);
	//for energy prediction
	start.WriteHtonU32(m_energyharvestsum);
	start.WriteHtonU32(m_energyconsumesum);
}

uint32_t NanoL3Header::Deserialize(Buffer::Iterator start) {
	Buffer::Iterator rbuf = start;

	m_source = rbuf.ReadNtohU32();
	m_destination = rbuf.ReadNtohU32();
	m_ttl = rbuf.ReadNtohU32();
	m_packetId = rbuf.ReadNtohU32();
	m_intqvalue = rbuf.ReadNtohU32();
	m_hopcount = rbuf.ReadNtohU32();
	m_previous = rbuf.ReadNtohU32();
	m_qhopcount = rbuf.ReadNtohU32();
	//for three probabilities
	m_deflectrate = rbuf.ReadNtohU32();
	m_droprate = rbuf.ReadNtohU32();
	m_energyrate = rbuf.ReadNtohU32();
	//for energy prediction
	m_energyharvestsum = rbuf.ReadNtohU32();
	m_energyconsumesum = rbuf.ReadNtohU32();

	return rbuf.GetDistanceFrom(start);
}

void NanoL3Header::Print(std::ostream &os) const {
	os << "src = " << m_source << " dst = " << m_destination << " ttl = "
			<< m_ttl << " pkt id = " << m_packetId;

}

void NanoL3Header::SetSource(uint32_t source) {
	m_source = source;
}

uint32_t NanoL3Header::GetSource() const {
	return m_source;
}

void NanoL3Header::SetDestination(uint32_t dst) {
	m_destination = dst;
}

uint32_t NanoL3Header::GetDestination() const {
	return m_destination;
}

void NanoL3Header::SetTtl(uint32_t ttl) {
	m_ttl = ttl;
}

void NanoL3Header::SetPacketId(uint32_t id) {
	m_packetId = id;
}

uint32_t NanoL3Header::GetTtl() const {
	return m_ttl;
}

uint32_t NanoL3Header::GetPacketId() const {
	return m_packetId;
}

void NanoL3Header::SetQvalue(uint32_t qvalue) {
	m_intqvalue = qvalue;
}

uint32_t NanoL3Header::GetQvalue() const {
	return m_intqvalue;
}
void NanoL3Header::SetHopCount(uint32_t hopcount) {
	m_hopcount = hopcount;
}

uint32_t NanoL3Header::GetHopCount() const {
	return m_hopcount;
}

void NanoL3Header::SetPrevious(uint32_t previous) {
	m_previous = previous;
}

uint32_t NanoL3Header::GetPrevious() const {
	return m_previous;
}

void NanoL3Header::SetQHopCount(uint32_t qhopcount) {
	m_qhopcount = qhopcount;
}

uint32_t NanoL3Header::GetQHopCount() const {
	return m_qhopcount;
}
//for three probabilities
void NanoL3Header::SetDeflectRate(uint32_t deflecterate) {
	m_deflectrate = deflecterate;
}

uint32_t NanoL3Header::GetDeflectRate() const {
	return m_deflectrate;
}

void NanoL3Header::SetDropRate(uint32_t droprate) {
	m_droprate = droprate;
}

uint32_t NanoL3Header::GetDropRate() const {
	return m_droprate;
}

void NanoL3Header::SetEnergyRate(uint32_t energyrate) {
	m_energyrate = energyrate;
}

uint32_t NanoL3Header::GetEnergyRate() const {
	return m_energyrate;
}

//for energy prediction
void NanoL3Header::SetEnergyHarvestSum(uint32_t energyharvestsum) {
	m_energyharvestsum = energyharvestsum;
}

uint32_t NanoL3Header::GetEnergyHarvestSum() const {
	return m_energyharvestsum;
}

void NanoL3Header::SetEnergyConsumeSum(uint32_t energyconsumesum) {
	m_energyconsumesum = energyconsumesum;
}

uint32_t NanoL3Header::GetEnergyConsumeSum() const {
	return m_energyconsumesum;
}
} // namespace ns3

