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

typedef unsigned char packetConfig[24];

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

        it++;
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

bool parseBuf(QByteArray* buf, packetConfig templ)
{
    return true;
}

bool sendSLS()
{
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString portCfgFile("C:\\Users\\Alexander Polonevich\\Documents\\su\\com.ini");
    QString packetCfgFile("C:\\Users\\Alexander Polonevich\\Documents\\su\\pac.ini");
    QString scriptFile("C:\\Users\\Alexander Polonevich\\Documents\\su\\script.txt");
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
    /*foreach(QString command, initScript)
    {
        err = true;
        emma.write(command.toUtf8());
        if (!emma.waitForBytesWritten())
            break;

        if(!emma.waitForReadyRead())
            break;

        QByteArray responseData = emma.readAll();
        while (emma.waitForReadyRead(10))
            responseData += emma.readAll();

        stdConsole.writeMessage(responseData);
        err = false;
    }*/

    if(!err)
        stdConsole.writeMessage("OK");
    else
    {
        stdConsole.writeMessage("FAIL");
        stdConsole.stopThread();
        return 2;
    }

    lsl::stream_info streamInfo(sender.toStdString().c_str(), "EEG", 2, rate, lsl::cf_float32);

    stdConsole.writeMessage("Loading stream description...");
    /*if(loadStreamInf(streamFile, &streamInfo))
        stdConsole.writeMessage("OK");
    else
        stdConsole.writeMessage("FAIL");
*/
    lsl::stream_outlet lslOut(streamInfo);

   //qDebug() << "channels" << streamInfo.channel_count();

    stdConsole.writeMessage("Starting interchange...");
    QByteArray buf;
    int32_t sample[2];
    while(true)
    {
        if(!stdConsole.getLatestCommand().isEmpty())
            break;

 /*       buf.append(emma.readAll());
        if(buf.isEmpty())
        {
            QThread::msleep(1);
            continue;
        }

        while(parseBuf(&buf, emmaPack))
              sendSLS();*/
        sample[0]=(rand()%1500)/500.0-1.5;
        sample[1]=(rand()%1500)/500.0-1.5;
        lslOut.push_sample(sample);
        QThread::msleep(1);

    }

    emma.close();
    stdConsole.writeMessage("*** Session Finished ***");
    stdConsole.stopThread();
    return a.exec();
}
