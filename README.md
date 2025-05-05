# Выполнение тестового задания для YADRO Импульс 2025
**Вариант тестового задания:**
1. Установить NS-3 и скомпилировать.
2. С помощью документации NS-3 сделать минимальный LTE сценарий.
   - Есть eNB и два абонента.
   - Трафик Full Buffer (пакеты идут в обе стороны бесконечно).
   - В LTE модуле сконфигурирован планировщик пакетов pf-ff-mac-scheduler.
   - В LTE модуле сконфигурирован вывод ключевых характеристик с Rlc и MAC уровня.
3. Запустить сценарий и получить вывод ключевых характеристик.

## Установка и компиляция NS-3
На сайте NS-3 имеется [руководство](https://www.nsnam.org/docs/installation/singlehtml/), где подробно описаны шаги для начала работы с симулятором.
- Первым делом клонируем Git-репозиторий с NS-3:
```
git clone https://gitlab.com/nsnam/ns-3-dev.git
```
- Переходим в склонированную папку:
```
cd ns-3-dev
```
- Следующим шагом является настройка сборки с включением примеров программ и тестов:
```
./ns3 configure --enable-examples --enable-tests
```
- Далее собираем проект:
```
./ns3 build
```
- Последним этапом запускаем модульные тесты для проверки успешности сборки:
```
./test.py
```

## Минимальный LTE сценарий
Для минимального LTE сценария создаём файл под названием `lte-simple-network.cc`, который будет располагаться в папке **scratch**.
### Подключение заголовков
```
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/lte-module.h>
#include <ns3/internet-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/applications-module.h>

using namespace ns3;
```
Для начала подключаем необходимые для дальнейшей работы заголовочные файлы, а также используем пространство имён ns3 для более удобного написания кода.

### Время симуляции 
```
Time simDuration = Seconds(5.0);
```
Здесь можно установить необходимое время симуляции модели.

### Создание помощников
```
Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
lteHelper->SetEpcHelper(epcHelper);
```
Для настройки LTE-сети и EPC создаём специальных помощников `LteHelper` и `PointToPointEpcHelper` и сообщаем LTE-помощнику об использовании EPC.

### Установка планировщика
```
lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
```
Устанавливаем планировщик пакетов `pf-ff-mac-scheduler` (Proportional Fair).

### Определение PGW и удалённого хоста
```
Ptr<Node> pgw = epcHelper->GetPgwNode();
NodeContainer remoteHostContainer;
remoteHostContainer.Create(1);
Ptr<Node> remoteHost = remoteHostContainer.Get(0);
InternetStackHelper internet;
internet.Install(remoteHostContainer);
```
В этой части мы получаем доступ к PGW через EPC-помощник. Затем создаём одиночный удалённый хост и устанавливаем на него сетевой стек.

### Соеднинение LTE-сети с удалённым хостом
```
PointToPointHelper p2ph;
p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
Ipv4AddressHelper ipv4h;
ipv4h.SetBase("1.0.0.0", "255.0.0.0");
Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);
```
Соединяем PGW и удалённый хост с помощью `PointToPointHelper`. Не забываем настроить параметры соединения: **DataRate**, **Mtu** и **Delay**. Также присваиваем IP-адреса каждому концу соеднинения.

### Настройка маршрутизации
```
Ipv4StaticRoutingHelper ipv4RoutingHelper;
Ptr<Ipv4StaticRouting> remoteHostStaticRouting;
remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
```
Чтобы удалённый хост знал, куда необходимо направлять пакеты, настраиваем маршрутизацию.

### Создание базовой станции и пользовательских устройств
```
NodeContainer enbNodes;
enbNodes.Create(1);
NodeContainer ueNodes;
ueNodes.Create(2);
```
Создаём одну базовую станцию и два пользовательских устройва.

### Установка модели передвижения
```
MobilityHelper mobility;
mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
mobility.Install(enbNodes);
mobility.Install(ueNodes);
```
Задаём для базовой станции и устройств модель передвижения `ConstantPositionMobilityModel`.

### Связывание базовой станции и пользовательских устройств с LTE-сетью
```
NetDeviceContainer enbDevs;
enbDevs = lteHelper->InstallEnbDevice(enbNodes);
NetDeviceContainer ueDevs;
ueDevs = lteHelper->InstallUeDevice(ueNodes);
```
Для работы устройств в нашей симуляции их нужно подключить к LTE-модели.

### Настройка IP-адресации и маршрутизации
```
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

lteHelper->Attach(ueDevs, enbDevs.Get(0));
```
Назначаем IP-адреса устройствам пользователей и определяем маршрут по умолчанию. Затем подключаем все устройства к базовой станции.

### Порты и контейнеры для настройки приложений
```
uint16_t dlPort = 1234;
uint16_t ulPort = 2000;
ApplicationContainer clientApps;
ApplicationContainer serverApps;
```
Данный код задаёт порты для downlink и uplink, а также создаёт контейнеры для клиентских и серверных приложений.

### Настройка Downlink и Uplink передачи
```
for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
{
    PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
    serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));
    UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
    dlClient.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
    dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));
    clientApps.Add(dlClient.Install(remoteHost));

    ++ulPort;
    PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
    serverApps.Add(ulPacketSinkHelper.Install(remoteHost));
    UdpClientHelper ulClient(remoteHostAddr, ulPort);
    ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
    ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));
    clientApps.Add(ulClient.Install(ueNodes.Get(u)));
}
```
Здесь происходит настройка приложений, которые будут передавать UDP-пакеты в downlink и uplink. Устанавливаем передачу пакетов раз в 1 мс с большим знаечнием максимума пакетов (иммитация ***Full Buffer*** трафика).

### Запуск приложений
```
serverApps.Start(Seconds(0.5));
clientApps.Start(Seconds(0.5));
```
Устанавливаем запуск приложений через 0.5 секунд начала симуляции.

### Вывод ключевых характеристик с RLC и MAC уровня
```
lteHelper->EnableMacTraces();
lteHelper->EnableRlcTraces();
```
Включаем вывод в файл ключевых характеристик с RLC и MAC уровня.

### Запуск и остановка симуляции
```
Simulator::Stop(simDuration);
Simulator::Run();
Simulator::Destroy();
```
Данный блок кода запускает симуляцию на определённое время (которое мы указывали в самом начале). 

Для создания данного сценария использовалась официальная [документация](https://www.nsnam.org/docs/models/html/lte.html) по модулю LTE в NS-3.

## Полученный результат симуляции
Как результат мы получаем файлы, которые содержат вывод ключевых характеристик с RLC и MAC уровня:
- [DLMacStats](https://github.com/IvanNoritsin/YADRO_Impulse_2025/blob/main/stats/DlMacStats.txt) — статистика MAC для Downlink
- [DlRlcStats](https://github.com/IvanNoritsin/YADRO_Impulse_2025/blob/main/stats/DlRlcStats.txt) — статистика RLC для Downlink
- [UlMacStats](https://github.com/IvanNoritsin/YADRO_Impulse_2025/blob/main/stats/UlMacStats.txt) — статистика MAC для Uplink
- [UlRlcStats](https://github.com/IvanNoritsin/YADRO_Impulse_2025/blob/main/stats/UlRlcStats.txt) — статистика RLC для Uplink
