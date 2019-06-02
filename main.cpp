#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QSerialPort>
#include <QSettings>
#include <QFileInfo>
#include <QMap>
#include <QList>
#include <QThread>
#include <QByteArrayMatcher>
#include <QtEndian>
#include <QBluetoothSocket>
#include <QTimer>

#include "LSL/include/lsl_cpp.h"
#include "pugixml/pugixml.hpp"

static bool diag = false;
#define DIAG if (diag) qWarning() << "[ DIAGN ]"

bool checkFileExists(QString filePath)
{
    bool ret = QFileInfo::exists(filePath) && QFileInfo(filePath).isFile();
    if(!ret)
        DIAG << "File doesn't exists or it's not a file";
    return ret;
}

bool loadScript(QString filePath, QStringList* script)
{
    if(!script)
        return false;

    if(!checkFileExists(filePath))
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        DIAG << "Can't open file";
        return false;
    }
    QString line;
    QTextStream textStream(&file);
    while (!textStream.atEnd())
    {
        line = textStream.readLine();
        line = line.left(line.indexOf("#"));
        if(!line.isEmpty())
            *script << line;
    }

    file.close();
    return true;
}

bool connectBT (QString filePath, QBluetoothSocket* socket, int timeout = 10000)
{
    if(!socket)
        return false;
    if(!checkFileExists(filePath))
        return false;

    QSettings btSettings(filePath, QSettings::IniFormat);

    bool result = true;

    QString macAddress("MAC");
    result = result && btSettings.contains(macAddress);
    DIAG << "BT MAC Address" << result;
    QBluetoothAddress remoteAddress(btSettings.value(macAddress).toString()); //"00:1D:43:9A:E0:76"
    DIAG << remoteAddress.toString();

    QString Uuid("UUID");
    result = result && btSettings.contains(Uuid);
    DIAG << "Service UUID" << result;
    QBluetoothUuid remoteUuid(btSettings.value(Uuid).toString()); //"00001101-0000-1000-8000-00805f9b34fb"
    DIAG << remoteUuid.toString();

    if(result)
    {
        DIAG << "An attempt to connect the socket:";
        QEventLoop loop;
        QObject::connect(socket, SIGNAL(connected()), &loop, SLOT(quit()));
        QTimer timer;
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.start(timeout);
        DIAG << "Timeout " << timeout/1000 << "s";

        socket->connectToService(remoteAddress, remoteUuid);
        loop.exec();

        result = (socket->state()==QBluetoothSocket::ConnectedState);

        if(!result && !socket->errorString().isEmpty())
            DIAG << socket->errorString();
    }
    return result;
}

bool connectCOM(QString filePath, QSerialPort* port)
{
    if(!port)
        return false;
    if(!checkFileExists(filePath))
        return false;

    QSettings comSettings(filePath, QSettings::IniFormat);

    bool result = true;

    QString portName("PortName");
    result = result && comSettings.contains(portName);
    DIAG << "Port Name" << result;
    port->setPortName(comSettings.value(portName).toString());
    DIAG << port->portName();

    QString baudRate("BaudRate");
    result = result &&
             comSettings.contains(baudRate) &&
             port->setBaudRate(comSettings.value(baudRate).toInt());
    DIAG << "Baud Rate" << result;

    QMap<QString, QSerialPort::DataBits> dataBitsMap = {{"Data5", QSerialPort::Data5},
                                                        {"Data6", QSerialPort::Data6},
                                                        {"Data7", QSerialPort::Data7},
                                                        {"Data8", QSerialPort::Data8},
                                                        {"UnknownDataBits", QSerialPort::UnknownDataBits}};
    QString dataBits("DataBits");
    result = result &&
             comSettings.contains(dataBits) &&
             dataBitsMap.contains(comSettings.value(dataBits).toString()) &&
             port->setDataBits(dataBitsMap[comSettings.value(dataBits).toString()]);
    DIAG << "Data Bits" << result;

    QMap<QString, QSerialPort::Parity> parityMap = {{"NoParity", QSerialPort::NoParity},
                                                    {"EvenParity", QSerialPort::EvenParity},
                                                    {"OddParity", QSerialPort::OddParity},
                                                    {"SpaceParity", QSerialPort::SpaceParity},
                                                    {"MarkParity", QSerialPort::MarkParity},
                                                    {"UnknownParity", QSerialPort::UnknownParity}};
    QString parity("Parity");
    result = result &&
             comSettings.contains(parity) &&
             parityMap.contains(comSettings.value(parity).toString()) &&
             port->setParity(parityMap[comSettings.value(parity).toString()]);
    DIAG << "Parity" << result;

    QMap<QString, QSerialPort::StopBits> stopBitsMap = {{"OneStop", QSerialPort::OneStop},
                                                        {"OneAndHalfStop", QSerialPort::OneAndHalfStop},
                                                        {"TwoStop", QSerialPort::TwoStop},
                                                        {"UnknownStopBits", QSerialPort::UnknownStopBits}};
    QString stopBits("StopBits");
    result = result &&
             comSettings.contains(stopBits) &&
             stopBitsMap.contains(comSettings.value(stopBits).toString()) &&
             port->setStopBits(stopBitsMap[comSettings.value(stopBits).toString()]);
    DIAG << "Stop Bits" << result;

    QMap<QString, QSerialPort::FlowControl> flowControlMap = {{"NoFlowControl", QSerialPort::NoFlowControl},
                                                              {"HardwareControl", QSerialPort::HardwareControl},
                                                              {"SoftwareControl", QSerialPort::SoftwareControl},
                                                              {"UnknownFlowControl", QSerialPort::UnknownFlowControl}};
    QString flowControl("FlowControl");
    result = result &&
             comSettings.contains(flowControl) &&
             flowControlMap.contains(comSettings.value(flowControl).toString()) &&
             port->setFlowControl(flowControlMap[comSettings.value(flowControl).toString()]);
    DIAG << "Flow Control" << result;


    if(result)
    {
        DIAG << "An attempt to open the port:";
        result &= port->open(QIODevice::ReadWrite);
    }
    return result;
}

typedef unsigned char packetData[65];
const int channels = 24;
typedef unsigned char packetConfig[channels];

bool loadPacketCfg(QString filePath, packetConfig pCfg)
{

    if(!pCfg)
        return false;
    if(!checkFileExists(filePath))
        return false;

    QSettings packetSettings(filePath, QSettings::IniFormat);

    QList<QPair<QString, unsigned char>> packetConfigList = {{"PacketCount", 1},
                                                             {"TimeStamp", 3},
                                                             {"LeadOff_Status1", 3},
                                                             {"LeadOff_Status2", 3},
                                                             {"EEG0chip1", 3},
                                                             {"EEG1chip1", 3},
                                                             {"EEG2chip1", 3},
                                                             {"EEG3chip1", 3},
                                                             {"EEG4chip1", 3},
                                                             {"EEG5chip1", 3},
                                                             {"EEG6chip1", 3},
                                                             {"EEG7chip1", 3},
                                                             {"EEG0chip2", 3},
                                                             {"EEG1chip2", 3},
                                                             {"EEG2chip2", 3},
                                                             {"EEG3chip2", 3},
                                                             {"EEG4chip2", 3},
                                                             {"EEG5chip2", 3},
                                                             {"EEG6chip2", 3},
                                                             {"EEG7chip2", 3},
                                                             {"ACCX", 2},
                                                             {"ACCY", 2},
                                                             {"ACCZ", 2},
                                                             {"CheckSumm", 1}};

    if(packetConfigList.count()!=sizeof(packetConfig))
    {
        DIAG << "Unexpected number of packet fields";
        return false;
    }

    QList<QPair<QString, unsigned char>>::iterator it = packetConfigList.begin();
    int pos = 0;
    while(it!=packetConfigList.end())
    {
        if(packetSettings.value(it->first, 0).toBool())
            pCfg[pos]=it->second;

        ++it;
        pos++;
    }

    return true;
}

bool loadStreamInf(QString filePath, lsl::stream_info* i)
{
    if(!i)
        return false;
    if(!checkFileExists(filePath))
        return false;

    QFile f(filePath);
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        DIAG << "Can't open file";
        return false;
    }

    QTextStream in(&f);
    QString xmlFile = in.readAll();
    i->from_xml(xmlFile.toStdString());

    return true;
}


int getPacketSize(packetConfig pCfg)
{
    int res = 2; // |0xA0|[DATA]|0xC0|
    for (size_t i=0; i<sizeof(packetConfig); i++)
        res+=pCfg[i];
    return res;
}

int getPacketChannels(packetConfig pCfg)
{
    int res = 0;
    for (size_t i=0; i<sizeof(packetConfig); i++)
        if(pCfg[i]>0)
            res++;
    return res;
}

int getOnChannels(packetConfig pCfg, QPair<int, int> range, int* indexOffset)
{
    int res = 0;
    *indexOffset =0;
    for (int i=0; i<=range.second; i++)
    {
        if(pCfg[i]>0)
        {
            if(i<range.first)
                (*indexOffset)++;
            else
                res++;
        }
    }
    return res;
}

bool getPacketData(QByteArray* buffer, int32_t* result, packetConfig curCfg, int expectedSize, QByteArrayMatcher* startSeq, QByteArrayMatcher* endSeq)
{
    int packStartIndex = startSeq->indexIn(*buffer);
    int packEndIndex = endSeq->indexIn(*buffer);

    if(packStartIndex<0 && packEndIndex<0)
    {
        buffer->clear(); // buffer size > expectedSize and neither packet start nor packet end found. Something strange.
        DIAG << "Buffer size > expectedSize and neither packet start nor packet end found";
        return false;
    }

    if(packStartIndex>=0 && packEndIndex>packStartIndex)
    {
        int stopLen = endSeq->pattern().size();
        if((packEndIndex-packStartIndex+(stopLen-2))!=expectedSize) // '-2' hardcoded, bc it is x0Dx0A bytes which is gonna turned off soon
        {
            buffer->remove(0, packEndIndex+stopLen);
            DIAG << "Unexpected packet size";
            return false;
        }

        char* chunk = buffer->data()+packStartIndex;
        int offset = 1;
        int resIndex= 0;

        for (size_t i=0; i<sizeof(packetConfig); i++)
        {
            if(curCfg[i]==0)
                continue;

            uint32_t chanVal = 0;

            memcpy(&chanVal, chunk+offset, curCfg[i]);
            qToBigEndian(chanVal, &chanVal);
            chanVal >>= (sizeof(uint32_t)-curCfg[i])*8;

            if (chanVal & 0x800000)
                chanVal |= 0xff000000;

            result[resIndex]=static_cast<int32_t>(chanVal);

            offset+=curCfg[i];
            resIndex++;
        }

        buffer->remove(0, packEndIndex+stopLen);
        return true;
    }

    if(buffer->length()>expectedSize*5)
    {
        buffer->clear();  // buffer size > expectedSize*5 and still no packet start or packet end found. Something strange.
        DIAG << "Buffer size > expectedSize*5 and still no packet start or packet end found";
    }
    else
        buffer->remove(0, qMax(packStartIndex, packEndIndex));
    return false;
}

bool getLSLframe(int32_t* dataChunk, int32_t* LSLframe, packetConfig packCfg, int indexOffset, QPair<int, int> chRange)
{
    int i = 0;
    for (int c=chRange.first; c<=chRange.second; c++)
    {
        if(packCfg[c]>0)
        {
            LSLframe[i] = dataChunk[indexOffset+i];
            i++;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("Emma terminal");
    QCoreApplication::setApplicationVersion("0.5");

    QCommandLineParser parser;
    parser.setApplicationDescription("COM -> LSL brocker");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("connection", "Path to .ini file with connection settings");
    parser.addPositionalArgument("packet", "Path to .ini file with packet configuration");
    parser.addPositionalArgument("script", "Path to .txt file with startup script");

    QCommandLineOption senderOption("sender", "Name of the stream. Describes the device that this stream makes available", "name");
    senderOption.setDefaultValue("Emma");
    QCommandLineOption rateOption("rate", "The sampling rate (in Hz) as advertised by the data source", "Hz");
    rateOption.setDefaultValue("0");
    parser.addOption(senderOption);
    parser.addOption(rateOption);

    QCommandLineOption BT("BT", "Connection via Bluetooth. COM port otherwise");
    parser.addOption(BT);

    QCommandLineOption verbose("verbose", "Diagnostic mode");
    parser.addOption(verbose);

    parser.process(a);
    const QStringList args = parser.positionalArguments();
    if(args.count()<3)
    {
        qWarning() << "Not enough arguments, run -h for help";
        return 0;
    }

    QString connCfgFile(args[0]);
    QString packetCfgFile(args[1]);
    QString scriptFile(args[2]);

    diag = parser.isSet(verbose);
    DIAG << "Diagnostic mode";

    bool btMode = parser.isSet(BT);
    DIAG << "Bluetooth connection";


    double rate = parser.value(rateOption).toDouble();
    DIAG << "Rate: " << rate;

    QString sender = parser.value(senderOption);
    DIAG << "Sender: " << sender;

    qInfo() << "[  INF  ] Emma term " << QCoreApplication::applicationVersion();

    QStringList initScript;
    QIODevice* emma;
    if(!btMode)
        emma = new QSerialPort();
    else
        emma = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);

    packetConfig emmaPack;
    memset(emmaPack, 0, sizeof(packetConfig));

    qInfo() << ("[  INF  ] Loadin packet config...");
    if(loadPacketCfg(packetCfgFile, emmaPack))
        qInfo() << ("[  INF  ] OK");
    else
        qInfo() << ("[WARNING] FAIL");

    qInfo() << ("[  INF  ] Loadin startup script...");
    if(loadScript(scriptFile, &initScript))
        qInfo() << ("[  INF  ] OK");
    else
        qInfo() << ("[WARNING] FAIL");

    qInfo() << ("[  INF  ] Connecting...");
    bool cr = true;
    if (strcmp(emma->metaObject()->className(), "QSerialPort") == 0)
        cr = connectCOM(connCfgFile, static_cast<QSerialPort*>(emma));
    else
        cr = connectBT(connCfgFile, static_cast<QBluetoothSocket*>(emma));
    if (cr)
        qInfo() << ("[  INF  ] OK");
    else
    {
        DIAG << "Can't connect";
        qInfo() << ("[WARNING] FAIL");
        return 1;
    }

    qInfo() << ("[  INF  ] Initializing...");
    const QString ema("EM+AUTO");
    if(initScript.first().left(ema.length())==ema)
    {
        DIAG << "Auto EM+...";
        initScript.removeFirst();
        initScript.prepend(strcmp(emma->metaObject()->className(), "QSerialPort") == 0?"EM+PC":"EM+BT");
        DIAG << initScript.first();
    }

    foreach(QString command, initScript)
    {
        qInfo() << ("    " + command);
        emma->write(command.toUtf8());

        if (strcmp(emma->metaObject()->className(), "QSerialPort") == 0)
            emma->waitForBytesWritten(3000);
    }
    qInfo() << ("[  INF  ] Complete");

    int ps = getPacketSize(emmaPack);
    int cc =getPacketChannels(emmaPack);
    const QPair<int, int> valuableChannels = qMakePair(4, 22);
    DIAG << "Vauable channels hardcoded: 4-22";
    int off = 0;
    int onChan = getOnChannels(emmaPack, valuableChannels, &off);

    lsl::stream_info streamInfo(sender.toStdString().c_str(), "EEG", onChan, rate, lsl::cf_float32);

    lsl::stream_outlet lslOut(streamInfo);

    qInfo() << ("[  INF  ] Starting interchange...");
    QByteArray buf;
    int32_t* sample = new int32_t[cc];

    QByteArrayMatcher startTemplate;
    startTemplate.setPattern(QByteArrayLiteral("\xA0"));
    QByteArrayMatcher endTemplate;
    endTemplate.setPattern(QByteArrayLiteral("\xC0\x0D\x0A"));

    int32_t* measuredData = new int32_t[onChan];
    memset(measuredData, 0, sizeof(int32_t)*static_cast<size_t>(onChan));

    QTimer quitGuard;
    quitGuard.start();
    QObject::connect(&quitGuard, &QTimer::timeout, [&quitGuard, &a] () {
        QTextStream inS(stdin);
        QString ggg = inS.readLine();

         if(inS.readLine()==QChar('q'))
             a.exit();
         else
             quitGuard.start();
        });

    if (strcmp(emma->metaObject()->className(), "QBluetoothSocket") == 0)
    {
        QObject::connect(static_cast<QBluetoothSocket*>(emma),
            QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error),
            [&](QBluetoothSocket::SocketError error){
            qInfo() << ("[WARNING] Connection issue");
            DIAG << error;
            if(error == QBluetoothSocket::RemoteHostClosedError)
            {
                qInfo() << ("[WARNING] Reconnect...");
                for (int i=0; i<10; i++)
                {
                    QString status = QString("[  INF  ] attempt %1 of 10").arg(i);
                    qInfo() << (status);

                    if(connectBT(connCfgFile, static_cast<QBluetoothSocket*>(emma), 1000))
                    {
                        qInfo() << ("[  INF  ] OK");
                        break;
                    }
                }
            }
        });
    }

    QObject::connect(emma, &QIODevice::readyRead, [&]( ){
        buf.append(emma->readAll());
        while(buf.length()>=(ps+2)) // we are looking for a origin packet + x0Dx0A bytes
            if(getPacketData(&buf, sample, emmaPack, ps, &startTemplate, &endTemplate))
            {
                getLSLframe(sample, measuredData, emmaPack, off, valuableChannels);
                if(lslOut.have_consumers())
                    lslOut.push_sample(measuredData);
            }
    });
    a.exec();

    delete [] sample;
    delete [] measuredData;
    emma->close();
    delete emma;
    qInfo() << ("[  INF  ] Session Finished");
    return 0;
}
