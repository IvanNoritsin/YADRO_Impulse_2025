#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/lte-module.h>
#include <ns3/internet-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/applications-module.h>

using namespace ns3;

int main(int argc, char *argv[])
{

    Time simDuration = Seconds(5.0); // Время симуляции

    // Создание помощников
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    // Выбор планировщика Proportional Fair
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");

    // Создание PGW и удалённого хоста
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // P2P для соединения LTE-сети и удалённого хоста
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting;
    remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // Создание 1 BS и 2 UEs
    NodeContainer enbNodes;
    enbNodes.Create(1);
    NodeContainer ueNodes;
    ueNodes.Create(2);

    // Модель передвижения
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);

    // Связываем BS и UEs с LTE-сетью
    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Настраиваем IP для UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);
        Ptr<Ipv4StaticRouting> ueStaticRouting;
        ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Подключаем UEs к базовой станции
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        // Настройка Downlink передачи
        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));
        UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
        dlClient.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
        dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));
        clientApps.Add(dlClient.Install(remoteHost));

        // Настройка Uplink передачи
        ++ulPort;
        PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));
        UdpClientHelper ulClient(remoteHostAddr, ulPort);
        ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
        ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));
        clientApps.Add(ulClient.Install(ueNodes.Get(u)));
    }

    serverApps.Start(Seconds(0.5));
    clientApps.Start(Seconds(0.5));

    // Логирование данных
    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();

    Simulator::Stop(simDuration);
    Simulator::Run();
    Simulator::Destroy();
    
    return 0;
}
