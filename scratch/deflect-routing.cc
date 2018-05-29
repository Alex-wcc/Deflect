/*
 * deflect-routing.cc
 *
 *  Created on: 2018-3-20
 *      Author: eric
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
#include "ns3/q-routing.h"
#include "ns3/netanim-module.h"

NS_LOG_COMPONENT_DEFINE("deflect-routing");

using namespace ns3;

void Run(int nbNanoNodes, double txRangeNanoNodes, int macType, int l3Type,
		int seed);

void PrintTXEvents(Ptr<OutputStreamWrapper> stream, int, int, int);
void PrintRXEvents(Ptr<OutputStreamWrapper> stream, int, int, int, int, double,
		int, int, int);
void PrintNodeStatus(Ptr<OutputStreamWrapper> stream, int, int, int, double,
		double, double, double);
void PrintPHYTXEvents(Ptr<OutputStreamWrapper> stream, int, int);
void PrintPHYCOLLEvents(Ptr<OutputStreamWrapper> stream, int, int);
void PrintSimulationTime(double duration);
// static void PrintPosition(std::ostream *os, std::string foo, NetDeviceContainer devs);
void PrintMemoryUsage(void);

int main(int argc, char *argv[]) {
	int nbNanoNodes = 25;
	double txRangeNanoNodes = 0.02;
	int macType = 2;
	int l3Type = 3; //deflection routing
	int seed = 1;

	CommandLine cmd;
	cmd.AddValue("seed", "seed", seed);
	cmd.AddValue("nbNanoNodes", "nbNanoNodes", nbNanoNodes);
	cmd.AddValue("txRangeNanoNodes", "txRangeNanoNodes", txRangeNanoNodes);
	cmd.AddValue("macType", "macType", macType);
	cmd.AddValue("l3Type", "l3Type", l3Type);
	cmd.Parse(argc, argv);

	Run(nbNanoNodes, txRangeNanoNodes, macType, l3Type, seed);
	return 0;
}

void Run(int nbNanoNodes, double txRangeNanoNodes, int macType, int l3Type,
		int seed) {

	std::cout << "START SIMULATION WITH: "
			"\n\t nbNanoNodes " << nbNanoNodes << "\n\t txRangeNanoNodes "
			<< txRangeNanoNodes
			<< "\n\t macType [1->Transparent; 2->BackoffBased] " << macType
			<< "\n\t l3Type [1->Flooding; 2->Random Routing;2->Defection Routing] "
			<< l3Type << "\n\t seed " << seed << std::endl;

	//timers
	Time::SetResolution(Time::FS);
	double duration = 500;

	//layout details
	double xrange = 0.05;
	double yrange = 0.05; //为什么y轴也是0呢？但是画出来的话明明有xy值呀。
    //double zrange = 0;
	//double zrange = 0.001;

	//physical details
	double pulseEnergy = 100e-12;
	double pulseDuration = 100;
	double pulseInterval = 10000;
	double powerTransmission = pulseEnergy / pulseDuration;

	//random variable generation
	srand(time(NULL));
	SeedManager::SetSeed(rand());
	Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable>();

	//helper definition
	NanoHelper nano;
	//nano.EnableLogComponents ();

	// Tracing
	AsciiTraceHelper asciiTraceHelper;
	std::stringstream file_outTX_s;
	file_outTX_s << "RES_TX" << "_N_" << nbNanoNodes << "_nTxRange_"
			<< txRangeNanoNodes << "_macType_" << macType << "_l3Type_"
			<< l3Type << "_seed_" << seed;
	std::stringstream file_outRX_s;
	file_outRX_s << "RES_RX" << "_N_" << nbNanoNodes << "_nTxRange_"
			<< txRangeNanoNodes << "_macType_" << macType << "_l3Type_"
			<< l3Type << "_seed_" << seed;
	std::stringstream file_NodeStatus_s;
	file_NodeStatus_s << "RES_NodeStatus" << "_N_" << nbNanoNodes
			<< "_nTxRange_" << txRangeNanoNodes << "_macType_" << macType
			<< "_l3Type_" << l3Type << "_seed_" << seed;
	std::stringstream file_outCorrectRX_s;
	std::stringstream file_outPHYTX_s;
	file_outPHYTX_s << "RES_PHYTX" << "_N_" << nbNanoNodes << "_nTxRange_"
			<< txRangeNanoNodes << "_macType_" << macType << "_l3Type_"
			<< l3Type << "_seed_" << seed;
	std::stringstream file_outPHYCOLL_s;
	file_outPHYCOLL_s << "RES_PHYCOLL" << "_N_" << nbNanoNodes << "_nTxRange_"
			<< txRangeNanoNodes << "_macType_" << macType << "_l3Type_"
			<< l3Type << "_seed_" << seed;

	std::string file_outTX = file_outTX_s.str();
	std::string file_outRX = file_outRX_s.str();
	std::string file_NodeStatus = file_NodeStatus_s.str();
	std::string file_outPHYTX = file_outPHYTX_s.str();
	std::string file_outPHYCOLL = file_outPHYCOLL_s.str();
	Ptr<OutputStreamWrapper> streamTX = asciiTraceHelper.CreateFileStream(
			file_outTX);
	Ptr<OutputStreamWrapper> streamRX = asciiTraceHelper.CreateFileStream(
			file_outRX);
	Ptr<OutputStreamWrapper> streamNodeStatus =
			asciiTraceHelper.CreateFileStream(file_NodeStatus);
	Ptr<OutputStreamWrapper> streamPHYTX = asciiTraceHelper.CreateFileStream(
			file_outPHYTX);
	Ptr<OutputStreamWrapper> streamPHYCOLL = asciiTraceHelper.CreateFileStream(
			file_outPHYCOLL);

	//network definition
	//NodeContainer n_routers;
	//NodeContainer n_gateways;
	NodeContainer n_nodes;
	//NetDeviceContainer d_gateways;
	NetDeviceContainer d_nodes;
	//NetDeviceContainer d_routers;

	n_nodes.Create(nbNanoNodes);
	d_nodes = nano.Install(n_nodes, NanoHelper::nanonode);

	//mobility need to revised it
	/*
	 MobilityHelper mobility;
	 mobility.SetMobilityModel("ns3::GaussMarkovMobilityModel", "Bounds",
	 BoxValue(Box(0, xrange, 0, yrange, 0, zrange)), "TimeStep",
	 TimeValue(Seconds(0.001)), "Alpha", DoubleValue(0), "MeanVelocity",
	 StringValue("ns3::UniformRandomVariable[Min=0.00|Max=0.001]"),
	 "MeanDirection",
	 StringValue("ns3::UniformRandomVariable[Min=0.00|Max=0.00]"),
	 "MeanPitch",
	 StringValue("ns3::UniformRandomVariable[Min=0.00|Max=0.001]"),
	 "NormalVelocity",
	 StringValue(
	 "ns3::NormalRandomVariable[Mean=0.00|Variance=0.00|Bound=0.0]"),
	 "NormalDirection",
	 StringValue(
	 "ns3::NormalRandomVariable[Mean=0.00|Variance=0.001|Bound=0.001]"),
	 "NormalPitch",
	 StringValue(
	 "ns3::NormalRandomVariable[Mean=0.00|Variance=0.001|Bound=0.001]"));
	 mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X",
	 StringValue("ns3::UniformRandomVariable[Min=0.0|Max=0.05]"),//RandomVariableValue (UniformVariable (0, xrange)),
	 "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=0.05]"),//RandomVariableValue (UniformVariable (0, yrange)),
	 "Z", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=0]"));	//RandomVariableValue (UniformVariable (0, zrange)));
	 */
	/*	 mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
	 "Bounds", RectangleValue (Rectangle (0, 0.05, 0, 0.05)));*/
//	 mobility.Install(n_nodes);
	//protocol stack
	for (uint16_t i = 0; i < d_nodes.GetN(); i++) {
		Ptr<NanoNodeDevice> dev = d_nodes.Get(i)->GetObject<NanoNodeDevice>();
		Ptr<MobilityModel> m = n_nodes.Get(i)->GetObject<MobilityModel>();
		//with no mobility
		Ptr<ConstantPositionMobilityModel> mm = CreateObject<
				ConstantPositionMobilityModel>();
		//保证每次的结构都相同，再进行对比。
		srand(i * 10 + 123);
		uint16_t x = (rand() % (nbNanoNodes - 1 + 1)) + 1;
		srand(i * 11 + 64);
		uint16_t y = (rand() % (nbNanoNodes - 1 + 1)) + 1;
		mm->SetPosition(
				Vector(x * (xrange / nbNanoNodes), y * (yrange / nbNanoNodes),
						0.0));
		nano.AddMobility(d_nodes.Get(i)->GetObject<NanoNodeDevice>()->GetPhy(),
				mm);

		Ptr<NanoRoutingEntity> routing;
		if (l3Type == 1) {
			Ptr<FloodingNanoRoutingEntity> routing2 = CreateObject<
					FloodingNanoRoutingEntity>();
			routing = routing2;
		} else if (l3Type == 2) {
			Ptr<RandomNanoRoutingEntity> routing2 = CreateObject<
					RandomNanoRoutingEntity>();
			routing = routing2;
		} else if (l3Type == 3) {
			Ptr<QRouting> routing3 = CreateObject<QRouting>();
			routing = routing3;
		} else {
		}
		routing->SetDevice(dev);
		dev->SetL3(routing);

		Ptr<NanoMacEntity> mac;
		if (macType == 1) {
			Ptr<TransparentNanoMacEntity> mac2 = CreateObject<
					TransparentNanoMacEntity>();
			mac = mac2;
		} else if (macType == 2) {
			Ptr<BackoffBasedNanoMacEntity> mac2 = CreateObject<
					BackoffBasedNanoMacEntity>();
			mac = mac2;
		} else {
		}
		mac->SetDevice(dev);
		dev->SetMac(mac);

		dev->GetPhy()->SetTransmissionRange(txRangeNanoNodes);
		dev->GetPhy()->SetTxPower(powerTransmission);
		dev->GetPhy()->GetObject<TsOokBasedNanoSpectrumPhy>()->SetPulseDuration(
				FemtoSeconds(pulseDuration));
		dev->GetPhy()->GetObject<TsOokBasedNanoSpectrumPhy>()->SetPulseInterval(
				FemtoSeconds(pulseInterval));
		dev->GetPhy()->TraceConnectWithoutContext("outPHYTX",
				MakeBoundCallback(&PrintPHYTXEvents, streamPHYTX));
		dev->GetPhy()->TraceConnectWithoutContext("outPHYCOLL",
				MakeBoundCallback(&PrintPHYCOLLEvents, streamPHYCOLL));

//物理层之后是节点本身
		double energy = 5.0;
		double maxenergy = 5.0;
		double harEnergyInterval = 0.1;
		double harEnergySpeed = 0.0;	// 330 /s
		//int reduceEnergy = 0;	//for random harvesting
		double EnergySendPerByte = 8.0 / 2.0 * 100.0 * 4.0 / 1000000.0;
		double EnergyReceivePerByte = 8.0 / 2.0 * 100.0 * 4.0 / 1000000.0 / 2.0;
		double PacketSize = 130;
		double ACKSize = 13;
		double NACKSize = 9;
		uint32_t buffersize = 1;

		dev->SetEnergySendPerByte(EnergySendPerByte);
		dev->SetEnergyReceivePerByte(EnergyReceivePerByte);
		dev->SetPacketSize(PacketSize);
		dev->SetACKSize(ACKSize);
		dev->SetNACKSize(NACKSize);

		dev->SetEnergyCapacity(energy);
		dev->SetMaxEnergy(maxenergy);
		dev->SetHarvestEnergyInterTime(harEnergyInterval);
		dev->SetHarEnergySpeed(harEnergySpeed);


		dev->SetBufferSize(buffersize);

	}

	//application
	double packetInterval = 3;

	for (int i = 0; i < nbNanoNodes; i++) {

		Ptr<MessageProcessUnit> mpu = CreateObject<MessageProcessUnit>();
		mpu->SetDevice(d_nodes.Get(i)->GetObject<SimpleNanoDevice>());
		d_nodes.Get(i)->GetObject<SimpleNanoDevice>()->SetMessageProcessUnit(
				mpu);
		//在message process里面进行了随机处理。
		mpu->SetInterarrivalTime(packetInterval);

		//d_nodes.Get(i)->GetObject<SimpleNanoDevice>()->SetEnergyCapacity()
		//生成随机目的地址；
		uint32_t dstId = random->GetValue(0, 25);
		mpu->SetDstId(dstId);

		//可以执行，从一个随机时间开始发送数据
		double startTime = random->GetValue(0.0, 15);
		Simulator::Schedule(Seconds(startTime),
				&MessageProcessUnit::CreteMessage, mpu);

        // let the node start harvest energy
		Ptr<SimpleNanoDevice> dev =
				d_nodes.Get(i)->GetObject<SimpleNanoDevice>();
		//可以执行，从一个随机时间开始收集能量
		double harstartTime = random->GetValue(0.0, 0.1);
		Simulator::Schedule(Seconds(harstartTime),
				&SimpleNanoDevice::HarvestEnergy, dev);
		mpu->TraceConnectWithoutContext("outTX",
				MakeBoundCallback(&PrintTXEvents, streamTX));
		mpu->TraceConnectWithoutContext("outRX",
				MakeBoundCallback(&PrintRXEvents, streamRX));
		mpu->TraceConnectWithoutContext("NodeStatus",
				MakeBoundCallback(&PrintNodeStatus, streamNodeStatus));
	}

	Simulator::Stop(Seconds(duration));
	Simulator::Schedule(Seconds(0.), &PrintSimulationTime, duration);
	Simulator::Schedule(Seconds(duration - 0.1), &PrintMemoryUsage);

	Simulator::Run();
	Simulator::Destroy();
}

void PrintMemoryUsage(void) {
	system(
			"ps aux | grep build/scratch/deflect-routing | head -1 | awk '{print $1, $4, $10}'");
}

void PrintTXEvents(Ptr<OutputStreamWrapper> stream, int id, int src, int src2) {
	*stream->GetStream() << src << " " << id << " " << " " << src2 << std::endl;
}

void PrintRXEvents(Ptr<OutputStreamWrapper> stream, int id, int size, int src,
		int thisNode, double delay, int ttl, int hopcount, int qhopcount) {
	*stream->GetStream() << src << " " << thisNode << " " << id << " "
			<< hopcount << " " << qhopcount << " " << size << " " << delay
			<< " " << std::endl;
}

void PrintNodeStatus(Ptr<OutputStreamWrapper> stream, int id, int energy,
		int maxenergy, double sendcount, double deflectcount,
		double receivecount, double receiveackcount) {
	*stream->GetStream() << id << " " << energy << " " << maxenergy << " "
			<< sendcount << " " << deflectcount << " " << receivecount << " "
			<< receiveackcount << " " << std::endl;
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

	double deltaT = duration / 10;
	int t = Simulator::Now().GetSeconds() / deltaT;

	double nexttime = deltaT * (t + 1);
	Simulator::Schedule(Seconds(nexttime), &PrintSimulationTime, duration);
}
