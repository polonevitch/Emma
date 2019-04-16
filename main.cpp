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

#include <console.h>

#include "LSL/include/lsl_cpp.h"
#include "pugixml/pugixml.hpp"

bool checkFileExists(QString filePath)
{
    return QFileInfo::exists(filePath) && QFileInfo(filePath).isFile();
}

bool loadScript(QString filePath, QStringList* script)
{
    if(!script)
        return false;

    if(!checkFileExists(filePath))
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

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

bool loadPortCfg(QString filePath, QSerialPort* port)
{
    if(!port)
        return false;
    if(!checkFileExists(filePath))
        return false;

    QSettings comSettings(filePath, QSettings::IniFormat);

    bool result = true;

    QString portName("PortName");
    result = result && comSettings.contains(portName);
    port->setPortName(comSettings.value(portName).toString());

    QString baudRate("BaudRate");
    result = result &&
             comSettings.contains(baudRate) &&
             port->setBaudRate(comSettings.value(baudRate).toInt());

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

    QMap<QString, QSerialPort::StopBits> stopBitsMap = {{"OneStop", QSerialPort::OneStop},
                                                        {"OneAndHalfStop", QSerialPort::OneAndHalfStop},
                                                        {"TwoStop", QSerialPort::TwoStop},
                                                        {"UnknownStopBits", QSerialPort::UnknownStopBits}};
    QString stopBits("StopBits");
    result = result &&
             comSettings.contains(stopBits) &&
             stopBitsMap.contains(comSettings.value(stopBits).toString()) &&
             port->setStopBits(stopBitsMap[comSettings.value(stopBits).toString()]);

    QMap<QString, QSerialPort::FlowControl> flowControlMap = {{"NoFlowControl", QSerialPort::NoFlowControl},
                                                              {"HardwareControl", QSerialPort::HardwareControl},
                                                              {"SoftwareControl", QSerialPort::SoftwareControl},
                                                              {"UnknownFlowControl", QSerialPort::UnknownFlowControl}};
    QString flowControl("FlowControl");
    result = result &&
             comSettings.contains(flowControl) &&
             flowControlMap.contains(comSettings.value(flowControl).toString()) &&
             port->setFlowControl(flowControlMap[comSettings.value(flowControl).toString()]);

    return result;
}

typedef unsigned char packetData[65];
const int channels = 24;
typedef unsigned char packetConfig[channels];
//const unsigned char channelsOffset[channels]={1, 2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 61, 63, 65};
//const unsigned char channelsSize[channels]={1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1};

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
        return false;

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
        return false;

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
        return false;
    }

    if(packStartIndex>=0 && packEndIndex>packStartIndex)
    {
        int stopLen = endSeq->pattern().size();
        if((packEndIndex-packStartIndex+(stopLen-2))!=expectedSize) // '-2' hardcoded, bc it is x0D0A bytes which is gonna turned off soon
        {
            buffer->remove(0, packEndIndex+stopLen);
            return false;
        }

        char* chunk = buffer->data()+packStartIndex;
        int offset = 1;
        int resIndex= 0;

        for (size_t i=0; i<sizeof(packetConfig); i++)
        {
            if(curCfg[i]==0)
                continue;

            int chanVal = 0;
            memcpy(&chanVal, chunk+offset, curCfg[i]);
            offset+=curCfg[i];
            result[resIndex]=chanVal;
            resIndex++;
        }

        buffer->remove(0, packEndIndex+stopLen);
        return true;
    }

    if(buffer->length()>expectedSize*5)
        buffer->clear();  // buffer size > expectedSize*5 and still no packet start or packet end found. Something strange.
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
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("COM -> LSL brocker");
    parser.addVersionOption();
    parser.addPositionalArgument("port", "Path to .ini file with com port esttings");
    parser.addPositionalArgument("packet", "Path to .ini file with packet configuration");
    parser.addPositionalArgument("script", "Path to .txt file with startup script");

    parser.process(a);
    const QStringList args = parser.positionalArguments();
    if(args.count()!=3)
        return 0;

    ///
    QFile bin("C:\\Users\\Alexander Polonevich\\Documents\\su\\packet.bin");
    if (!bin.open(QFile::ReadOnly))
        return 1;
    ///

    QString portCfgFile(args[0]);
    QString packetCfgFile(args[1]);
    QString scriptFile(args[2]);
    QString streamFile("C:\\Users\\Alexander Polonevich\\Documents\\su\\stream.xml");


    QString sender("Emma");
    QString content("Content");
    double rate = lsl::IRREGULAR_RATE;

    console stdConsole;
    stdConsole.start();
    stdConsole.writeMessage("Emma term v0.1");

    QStringList initScript;
    QSerialPort emma;

    packetConfig emmaPack;
    memset(emmaPack, 0, sizeof(packetConfig));

    stdConsole.writeMessage("Loadin port config...");
    if(loadPortCfg(portCfgFile, &emma))
        stdConsole.writeMessage("OK");
    else
        stdConsole.writeMessage("FAIL");

    stdConsole.writeMessage("Loadin packet config...");
    if(loadPacketCfg(packetCfgFile, emmaPack))
        stdConsole.writeMessage("OK");
    else
        stdConsole.writeMessage("FAIL");

    stdConsole.writeMessage("Loadin startup script...");
    if(loadScript(scriptFile, &initScript))
        stdConsole.writeMessage("OK");
    else
        stdConsole.writeMessage("FAIL");

    stdConsole.writeMessage("Opening port...");
    if (emma.open(QIODevice::ReadWrite))
        stdConsole.writeMessage("OK");
    else
    {
        stdConsole.writeMessage("FAIL");
        stdConsole.stopThread();
        return 1;
    }

    stdConsole.writeMessage("Initializing...");
    bool err = false;
    foreach(QString command, initScript)
    {
        stdConsole.writeMessage("    " + command);
        err = true;
        emma.write(command.toUtf8());
        if (!emma.waitForBytesWritten())
            break;

        if(!emma.waitForReadyRead())
            break;

        QByteArray responseData = emma.readAll();
        while (emma.waitForReadyRead(10))
            responseData += emma.readAll();

        stdConsole.writeMessage("    " + responseData);
        err = false;
    }

    if(!err)
        stdConsole.writeMessage("OK");
    else
    {
        stdConsole.writeMessage("FAIL");
        stdConsole.stopThread();
        return 2;
    }

    int ps = getPacketSize(emmaPack);
    int cc =getPacketChannels(emmaPack);
    const QPair<int, int> valuableChannels = qMakePair(4, 22);
    int off = 0;
    int onChan = getOnChannels(emmaPack, valuableChannels, &off);

    lsl::stream_info streamInfo(sender.toStdString().c_str(), "EEG", onChan, rate, lsl::cf_float32);

    //stdConsole.writeMessage("Loading stream description...");
    /*if(loadStreamInf(streamFile, &streamInfo))
        stdConsole.writeMessage("OK");
    else
        stdConsole.writeMessage("FAIL");
*/
    lsl::stream_outlet lslOut(streamInfo);

    stdConsole.writeMessage("Starting interchange...");
    QByteArray buf;//=bin.readAll();
    int32_t* sample = new int32_t[cc];

    QByteArrayMatcher startTemplate;
    startTemplate.setPattern(QByteArrayLiteral("\xA0"));
    QByteArrayMatcher endTemplate;
    endTemplate.setPattern(QByteArrayLiteral("\xC0\x0D\x0A"));

    int32_t* measuredData = new int32_t[onChan];
    memset(measuredData, 0, sizeof(int32_t)*static_cast<size_t>(onChan));

    while(true)
    {
        if(!stdConsole.getLatestCommand().isEmpty())
            break;
        if(!emma.waitForReadyRead())
            continue;
        buf.append(emma.readAll());
        //if(buf.isEmpty())
        //{
        //    QThread::msleep(1);
        //    continue;
        //}
//int pc = 0;
        while(buf.length()>=ps)
            if(getPacketData(&buf, sample, emmaPack, ps, &startTemplate, &endTemplate))
            {
                getLSLframe(sample, measuredData, emmaPack, off, valuableChannels);
                lslOut.push_sample(measuredData);
                for (int i=0; i<onChan; i++)
                    qDebug() << measuredData[i];
                qDebug() << "-----------------";
            }
             //   pc++;
        //qDebug() << pc;

    }

    delete [] sample;
    delete [] measuredData;
    emma.close();
    stdConsole.writeMessage("*** Session Finished ***");
    stdConsole.stopThread();
    return a.exec();
}
