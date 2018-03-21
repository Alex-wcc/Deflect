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
 *
 *
 * run:
 *
 * ./waf --run "scratch/health-care --seed=1 --nbNanoNodes=200 --nbNanoRouters=50 --nbNanoGateways=1 --txRangeNanoNodes=0.01 --macType=1 --l3Type=1
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/internet-module.h"
#include <iostream>
#include "ns3/global-route-manager.h"
#include "ns3/nano-mac-entity.h"
#include "ns3/nano-mac-queue.h"
#include "ns3/nano-spectrum-channel.h"
#include "ns3/nano-spectrum-phy.h"
#include "ns3/nano-spectrum-signal-parameters.h"
#include "ns3/nano-helper.h"
#include "ns3/nano-spectrum-value-helper.h"
#include "ns3/simple-nano-device.h"
#include "ns3/nanointerface-nano-device.h"
#include "ns3/nanorouter-nano-device.h"
#include "ns3/nanonode-nano-device.h"
#include "ns3/backoff-based-nano-mac-entity.h"
#include "ns3/seq-ts-header.h"
#include "ns3/ts-ook-based-nano-spectrum-phy.h"
#include "ns3/mobility-model.h"
#include "ns3/message-process-unit.h"
#include "ns3/transparent-nano-mac-entity.h"
#include "ns3/random-nano-routing-entity.h"
#include "ns3/flooding-nano-routing-entity.h"
NS_LOG_COMPONENT_DEFINE ("health-care");

using namespace ns3;



void Run(int nbNanoNodes, int nbNanoRouters, int nbNanoGateways, double txRangeNanoNodes, double txRangeNanoRouter, int macType, int l3Type, int seed);

void PrintTXEvents(Ptr<OutputStreamWrapper> stream, int, int);
void PrintRXEvents(Ptr<OutputStreamWrapper> stream, int, int, int, int, double);
void PrintPHYTXEvents(Ptr<OutputStreamWrapper> stream, int, int);
void PrintPHYCOLLEvents(Ptr<OutputStreamWrapper> stream, int, int);
void PrintSimulationTime(double duration);
// static void PrintPosition(std::ostream *os, std::string foo, NetDeviceContainer devs);
void PrintMemoryUsage(void);



int main(int argc, char *argv[]) {

	int nbNanoNodes = 200;
	int nbNanoRouters = 50;
	int nbNanoGateways = 1;
	double txRangeNanoNodes = 0.01;
	double txRangeNanoRouter = 0.02;
	int macType = 1;
	int l3Type = 1;
	int seed = 1;

	CommandLine cmd;
	cmd.AddValue("seed", "seed", seed);
	cmd.AddValue("nbNanoNodes", "nbNanoNodes", nbNanoNodes);
	cmd.AddValue("nbNanoRouters", "nbNanoRouters", nbNanoRouters);
	cmd.AddValue("nbNanoGateways", "nbNanoGateways", nbNanoGateways);
	cmd.AddValue("txRangeNanoNodes", "txRangeNanoNodes", txRangeNanoNodes);
	cmd.AddValue("txRangeNanoRouter", "txRangeNanoRouter", txRangeNanoNodes);
	cmd.AddValue("macType", "macType", macType);
	cmd.AddValue("l3Type", "l3Type", l3Type);
	cmd.Parse(argc, argv);

	Run(nbNanoNodes, nbNanoRouters, nbNanoGateways, txRangeNanoNodes, txRangeNanoRouter, macType, l3Type, seed);

	return 0;
}

void Run(int nbNanoNodes, int nbNanoRouters, int nbNanoGateways, double txRangeNanoNodes, double txRangeNanoRouter, int macType, int l3Type, int seed)
{

	std::cout << "START SIMULATION WITH: "
			"\n\t nbNanoNodes " << nbNanoNodes <<
			"\n\t nbNanoRouters " << nbNanoRouters <<
			"\n\t nbNanoGateways " << nbNanoGateways <<
			"\n\t txRangeNanoNodes " << txRangeNanoNodes <<
			"\n\t txRangeNanoRouter "<< txRangeNanoRouter <<
			"\n\t macType [1->Transparent; 2->BackoffBased] "<< macType <<
			"\n\t l3Type [1->Flooding; 2->Random Routing] "<< l3Type <<
			"\n\t seed " << seed << std::endl;

	//timers
	Time::SetResolution(Time::FS);
	double duration = 5;

	//layout details
	double xrange = 1;
	double yrange = 0.001;
	double zrange = 0.001;

	//physical details
	double pulseEnergy = 100e-12;
	double pulseDuration = 100;
	double pulseInterval = 10000;
	double powerTransmission = pulseEnergy / pulseDuration;

	//random variable generation
	srand(time(NULL));
	SeedManager::SetSeed(rand());
  Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable> ();

	//helper definition
	NanoHelper nano;
	//nano.EnableLogComponents ();



	// Tracing
	AsciiTraceHelper asciiTraceHelper;
	std::stringstream file_outTX_s;
	file_outTX_s << "RES_TX" << "_N_" << nbNanoNodes << "_R_" << nbNanoRouters << "_G_" << nbNanoGateways <<
			"_nTxRange_" << txRangeNanoNodes << "_macType_" << macType << "_l3Type_" << l3Type << "_seed_" << seed;
	std::stringstream file_outRX_s;
	file_outRX_s << "RES_RX" << "_N_" << nbNanoNodes << "_R_" << nbNanoRouters << "_G_" << nbNanoGateways <<
			"_nTxRange_" << txRangeNanoNodes << "_macType_" << macType << "_l3Type_" << l3Type << "_seed_" << seed;
	std::stringstream file_outCorrectRX_s;
	std::stringstream file_outPHYTX_s;
	file_outPHYTX_s << "RES_PHYTX" << "_N_" << nbNanoNodes << "_R_" << nbNanoRouters << "_G_" << nbNanoGateways <<
			"_nTxRange_" << txRangeNanoNodes << "_macType_" << macType << "_l3Type_" << l3Type << "_seed_" << seed;
	std::stringstream file_outPHYCOLL_s;
	file_outPHYCOLL_s << "RES_PHYCOLL" << "_N_" << nbNanoNodes << "_R_" << nbNanoRouters << "_G_" << nbNanoGateways <<
			"_nTxRange_" << txRangeNanoNodes << "_macType_" << macType << "_l3Type_" << l3Type << "_seed_" << seed;

	std::string file_outTX = file_outTX_s.str();
	std::string file_outRX = file_outRX_s.str();
	std::string file_outPHYTX = file_outPHYTX_s.str();
	std::string file_outPHYCOLL = file_outPHYCOLL_s.str();
	Ptr<OutputStreamWrapper> streamTX = asciiTraceHelper.CreateFileStream(
			file_outTX);
	Ptr<OutputStreamWrapper> streamRX = asciiTraceHelper.CreateFileStream(
			file_outRX);
	Ptr<OutputStreamWrapper> streamPHYTX = asciiTraceHelper.CreateFileStream(
			file_outPHYTX);
	Ptr<OutputStreamWrapper> streamPHYCOLL = asciiTraceHelper.CreateFileStream(
			file_outPHYCOLL);




	//network definition
	NodeContainer n_routers;
	NodeContainer n_gateways;
	NodeContainer n_nodes;
	NetDeviceContainer d_gateways;
	NetDeviceContainer d_nodes;
	NetDeviceContainer d_routers;

	n_gateways.Create(nbNanoGateways);
	d_gateways = nano.Install(n_gateways, NanoHelper::nanointerface);
	n_routers.Create(nbNanoRouters);
	d_routers = nano.Install(n_routers, NanoHelper::nanorouter);
	n_nodes.Create(nbNanoNodes);
	d_nodes = nano.Install(n_nodes, NanoHelper::nanonode);


	//mobility
	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
			"Bounds", BoxValue (Box (0, xrange, 0, yrange, 0, zrange)),
			"TimeStep", TimeValue (Seconds (0.001)),
			"Alpha", DoubleValue (0),
			"MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=0.2|Max=0.2]"),
			"MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=0]"),
			"MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
			"NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
			"NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
			"NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"));
	mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
			"X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=0.15]"),//RandomVariableValue (UniformVariable (0, xrange)),
			"Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=0.001]"),//RandomVariableValue (UniformVariable (0, yrange)),
			"Z", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=0.001]"));//RandomVariableValue (UniformVariable (0, zrange)));
	mobility.Install (n_nodes);
	//std::string logFile = "nanonodeposition.log";
	//std::ofstream os;
	//os.open(logFile.c_str());
	//Simulator::Schedule(Seconds(0.001), &PrintPosition, &os, logFile, d_nodes);

	//protocol stack
	for (int x = 0; x < nbNanoGateways; x++) {

		Ptr<NanoInterfaceDevice> interface = d_gateways.Get(x)->GetObject<
				NanoInterfaceDevice> ();
		Ptr<ConstantPositionMobilityModel> m = CreateObject<
				ConstantPositionMobilityModel> ();

		m->SetPosition(Vector(xrange/2, 0.0, 0.0));

		nano.AddMobility(interface->GetPhy(), m);

		Ptr<NanoRoutingEntity> routing;
		if (l3Type == 1) {
			Ptr<FloodingNanoRoutingEntity> routing2 = CreateObject<
					FloodingNanoRoutingEntity> ();
			routing = routing2;
		} else if (l3Type == 2) {
			Ptr<RandomNanoRoutingEntity> routing2 = CreateObject<
					RandomNanoRoutingEntity> ();
			routing = routing2;
		} else {
		}
		routing->SetDevice(interface);
		interface->SetL3(routing);

		Ptr<NanoMacEntity> mac;
		if (macType == 1) {
			Ptr<TransparentNanoMacEntity> mac2 = CreateObject<
					TransparentNanoMacEntity> ();
			mac = mac2;
		} else if (macType == 2) {
			Ptr<BackoffBasedNanoMacEntity> mac2 = CreateObject<
					BackoffBasedNanoMacEntity> ();
			mac = mac2;
		} else {
		}
		mac->SetDevice(interface);
		interface->SetMac(mac);

		interface->GetPhy()->SetTxPower(powerTransmission);
		interface->GetPhy()->SetTransmissionRange(txRangeNanoRouter);
		interface->GetPhy()->GetObject<TsOokBasedNanoSpectrumPhy> ()->SetPulseDuration(
				FemtoSeconds(pulseDuration));
		interface->GetPhy()->GetObject<TsOokBasedNanoSpectrumPhy> ()->SetPulseInterval(
				FemtoSeconds(pulseInterval));

		interface->GetPhy()->TraceConnectWithoutContext("outPHYTX",
				MakeBoundCallback(&PrintPHYTXEvents, streamPHYTX));
		interface->GetPhy()->TraceConnectWithoutContext("outPHYCOLL",
				MakeBoundCallback(&PrintPHYCOLLEvents, streamPHYCOLL));
	}

	for (int x = 0; x < nbNanoRouters; x++) {

		Ptr<NanoRouterDevice> router = d_routers.Get(x)->GetObject<
				NanoRouterDevice> ();
		Ptr<ConstantPositionMobilityModel> m = CreateObject<
				ConstantPositionMobilityModel> ();
		m->SetPosition(Vector(x * (xrange / nbNanoRouters), 0.0, 0.0));
		nano.AddMobility(router->GetPhy(), m);

		Ptr<NanoRoutingEntity> routing;
		if (l3Type == 1) {
			Ptr<FloodingNanoRoutingEntity> routing2 = CreateObject<
					FloodingNanoRoutingEntity> ();
			routing = routing2;
		} else if (l3Type == 2) {
			Ptr<RandomNanoRoutingEntity> routing2 = CreateObject<
					RandomNanoRoutingEntity> ();
			routing = routing2;
		} else {
		}
		routing->SetDevice(router);
		router->SetL3(routing);

		Ptr<NanoMacEntity> mac;
		if (macType == 1) {
			Ptr<TransparentNanoMacEntity> mac2 = CreateObject<
					TransparentNanoMacEntity> ();
			mac = mac2;
		} else if (macType == 2) {
			Ptr<BackoffBasedNanoMacEntity> mac2 = CreateObject<
					BackoffBasedNanoMacEntity> ();
			mac = mac2;
		} else {
		}
		mac->SetDevice(router);
		router->SetMac(mac);

		router->GetPhy()->SetTxPower(powerTransmission);
		router->GetPhy()->SetTransmissionRange(txRangeNanoRouter);
		router->GetPhy()->GetObject<TsOokBasedNanoSpectrumPhy> ()->SetPulseDuration(
				FemtoSeconds(pulseDuration));
		router->GetPhy()->GetObject<TsOokBasedNanoSpectrumPhy> ()->SetPulseInterval(
				FemtoSeconds(pulseInterval));

		router->GetPhy()->TraceConnectWithoutContext("outPHYTX",
				MakeBoundCallback(&PrintPHYTXEvents, streamPHYTX));
		router->GetPhy()->TraceConnectWithoutContext("outPHYCOLL",
				MakeBoundCallback(&PrintPHYCOLLEvents, streamPHYCOLL));

	}

	for (uint32_t i = 0; i < d_nodes.GetN(); i++) {
		Ptr<MobilityModel> m = n_nodes.Get(i)->GetObject<MobilityModel> ();
		nano.AddMobility(
				d_nodes.Get(i)->GetObject<NanoNodeDevice> ()->GetPhy(), m);
		Ptr<NanoNodeDevice> dev = d_nodes.Get(i)->GetObject<NanoNodeDevice> ();

		Ptr<NanoRoutingEntity> routing;
		if (l3Type == 1) {
			Ptr<FloodingNanoRoutingEntity> routing2 = CreateObject<
					FloodingNanoRoutingEntity> ();
			routing = routing2;
		} else if (l3Type == 2) {
			Ptr<RandomNanoRoutingEntity> routing2 = CreateObject<
					RandomNanoRoutingEntity> ();
			routing = routing2;
		} else {
		}
		routing->SetDevice(dev);
		dev->SetL3(routing);

		Ptr<NanoMacEntity> mac;
		if (macType == 1) {
			Ptr<TransparentNanoMacEntity> mac2 = CreateObject<
					TransparentNanoMacEntity> ();
			mac = mac2;
		} else if (macType == 2) {
			Ptr<BackoffBasedNanoMacEntity> mac2 = CreateObject<
					BackoffBasedNanoMacEntity> ();
			mac = mac2;
		} else {
		}
		mac->SetDevice(dev);
		dev->SetMac(mac);

		dev->GetPhy()->SetTransmissionRange(txRangeNanoNodes);
		dev->GetPhy()->SetTxPower(powerTransmission);
		dev->GetPhy()->GetObject<TsOokBasedNanoSpectrumPhy> ()->SetPulseDuration(
				FemtoSeconds(pulseDuration));
		dev->GetPhy()->GetObject<TsOokBasedNanoSpectrumPhy> ()->SetPulseInterval(
				FemtoSeconds(pulseInterval));

		dev->GetPhy()->TraceConnectWithoutContext("outPHYTX",
				MakeBoundCallback(&PrintPHYTXEvents, streamPHYTX));
		dev->GetPhy()->TraceConnectWithoutContext("outPHYCOLL",
				MakeBoundCallback(&PrintPHYCOLLEvents, streamPHYCOLL));

	}





	//application
	double packetInterval = 0.1;

	for (int i = 0; i < nbNanoNodes; i++) {

		Ptr<MessageProcessUnit> mpu = CreateObject<MessageProcessUnit> ();
		mpu->SetDevice(d_nodes.Get(i)->GetObject<SimpleNanoDevice> ());
		d_nodes.Get(i)->GetObject<SimpleNanoDevice> ()->SetMessageProcessUnit(
				mpu);
		mpu->SetInterarrivalTime(packetInterval);

		double startTime = random->GetValue(0.0, 0.1);
		Simulator::Schedule(Seconds(startTime),
				&MessageProcessUnit::CreteMessage, mpu);
		mpu->TraceConnectWithoutContext("outTX",
				MakeBoundCallback(&PrintTXEvents, streamTX));
		mpu->TraceConnectWithoutContext("outRX",
				MakeBoundCallback(&PrintRXEvents, streamRX));
	}

	for (int i = 0; i < nbNanoGateways; i++) {
		Ptr<MessageProcessUnit> mpu = CreateObject<MessageProcessUnit> ();
		mpu->SetDevice(d_gateways.Get(i)->GetObject<SimpleNanoDevice> ());
		d_gateways.Get(i)->GetObject<SimpleNanoDevice> ()->SetMessageProcessUnit(
				mpu);
		mpu->SetDevice(d_gateways.Get(i)->GetObject<SimpleNanoDevice> ());
		d_gateways.Get(i)->GetObject<SimpleNanoDevice> ()->SetMessageProcessUnit(
				mpu);
		mpu->TraceConnectWithoutContext("outTX",
				MakeBoundCallback(&PrintTXEvents, streamTX));
		mpu->TraceConnectWithoutContext("outRX",
				MakeBoundCallback(&PrintRXEvents, streamRX));
	}

	Simulator::Stop(Seconds(duration));
	Simulator::Schedule(Seconds(0.), &PrintSimulationTime, duration);
	Simulator::Schedule(Seconds(duration - 0.1), &PrintMemoryUsage);

	Simulator::Run();
	Simulator::Destroy();
}





/*
static void PrintPosition(std::ostream *os, std::string foo,
		NetDeviceContainer devs) {
	int num = devs.GetN();
	for (int i = 0; i < num; i++) {
		Ptr<SimpleNanoDevice> d = devs.Get(i)->GetObject<SimpleNanoDevice> ();
		Ptr<MobilityModel> m = d->GetPhy()->GetMobility();
		Vector pos = m->GetPosition();
		*os << Simulator::Now().GetSeconds() << " " << d->GetNode()->GetId()
								<< " " << pos.x << " " << pos.y << " " << pos.z << std::endl;
	}
	Simulator::Schedule(Seconds(0.1), &PrintPosition, os, foo, devs);
}
*/

void PrintMemoryUsage(void) {
	system(
			"ps aux | grep build/scratch/health-care | head -1 | awk '{print $1, $4, $10}'");
}


void PrintTXEvents(Ptr<OutputStreamWrapper> stream, int id, int src) {
	*stream->GetStream() << src << " " << id << std::endl;
}

void PrintRXEvents(Ptr<OutputStreamWrapper> stream, int id, int size, int src,
		int thisNode, double delay) {
	*stream->GetStream()  << thisNode << " " << id << " " << size << " " << delay
			<< std::endl;
}

void PrintPHYCOLLEvents(Ptr<OutputStreamWrapper> stream, int id, int thisNode) {
	*stream->GetStream() << thisNode << " " << id << std::endl;
}

void PrintPHYTXEvents(Ptr<OutputStreamWrapper> stream, int id, int src) {
	*stream->GetStream() << src << " " << id << std::endl;
}

void PrintSimulationTime(double duration) {

	double percentage = (100. * Simulator::Now().GetSeconds()) / duration;
	std::cout << "*** " << percentage << " *** " << std::endl;

	double deltaT = duration/10;
	int t = Simulator::Now().GetSeconds() / deltaT;

	double nexttime = deltaT * (t+1);
	Simulator::Schedule(Seconds(nexttime), &PrintSimulationTime, duration);
}
