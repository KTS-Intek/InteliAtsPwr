#ifndef INTELIATSPWRDECODER_H
#define INTELIATSPWRDECODER_H

#include <QObject>
#include <QVariantHash>
#include <QDebug>
#include <QTime>


#include "meterplugintypes.h"
#include "modbusprocessor4inteliatspwr.h"


///[!] meter-plugin-shared
#include "shared/meterpluginhelper.h"
#include "shared/addmeterdatahelper.h"

///[!] modbus-base
#include "modbus-shared/modbusmessanger.h"

class InteliAtsPwrDecoder : public QObject
{
    Q_OBJECT
public:
    explicit InteliAtsPwrDecoder(QObject *parent = nullptr);

    bool verboseMode ;
    ErrsStrct lasterrwarn;
    quint8 lastAddr;
    QByteArray lastGoodArr;
    QTime plgtimeg;

//    QMap<QString,qint64> mapni2nextsnupdate;


    static QByteArray getDefPasswd() ;

    static QStringList getSupportedEnrg(const quint8 &code);

    static bool isPollCodeSupported(const quint16 &pollCode, QVariantHash &hashTmpData);

    static bool isPollCodeSupported(const quint16 &pollCode);

    QByteArray getMyModbusReadRegisterMessage(const quint8 &address, const quint16 &registerno, const quint16 &len);


    QByteArray meterNiFromConstHash(const QVariantHash &hashConstData);

    QByteArray getReadMessage(const QByteArray &ni, const quint16 &startcommand, const quint16 &lastcommand);

    QByteArray getReadMessage(const QVariantHash &hashConstData, const quint16 &startcommand, const quint16 &lastcommand);

    QVariantHash decodeEnd(const QVariantHash &decodehash, const ErrCounters &errwarns, quint16 &step, QVariantHash &hashTmpData);


    Mess2meterRezult message2meter(const Mess2meterArgs &pairAboutMeter);

    QVariantHash decodeMeterData(const DecodeMeterMess &threeHash);



    QVariantHash preparyVoltage(const QVariantHash &hashConstData, QVariantHash &hashTmpData);

    QVariantHash fullVoltage(const MessageValidatorResult &decoderesult, const QVariantHash &hashTmpData, quint16 &step, ErrCounters &warnerr);


    QVariantHash preparyTotalEnrg(const QVariantHash &hashConstData, QVariantHash &hashTmpData);
    QVariantHash fullTotalEnrg(const MessageValidatorResult &decoderesult, const QVariantHash &hashTmpData, quint16 &step, ErrCounters &warnerr);

    QVariantHash preparyMeterState(const QVariantHash &hashConstData, QVariantHash &hashTmpData);
    QVariantHash fullMeterState(const MessageValidatorResult &decoderesult, const QVariantHash &hashTmpData, quint16 &step, ErrCounters &warnerr);

    void insertDefaultHashMessageValues(QVariantHash &hashMessage);


    void insertLists2hash(QVariantHash &hash, const QStringList &lkeys, const QStringList &ldata);



    void addDevVersion(QVariantHash &hashTmpData, const ModbusAnswerList &listMeterMessage);

    QHash<QString,QString> addVoltageLoadFreqValues(const ModbusAnswerList &listMeterMessage);

    QMap<QString, ModbusAnswerList> normalizeBranchCircuitEnergy(const ModbusAnswerList &inputListMeterMess);


    QStringList getOneBranchCircuitValues(const ModbusAnswerList &listMeterMessage);
    QStringList getOneBranchCircuitKeys();
    QStringList getOneBranchCircuitXKeys();


    QStringList getOneBranchCircuitEnergy(const ModbusAnswerList &listMeterMessage);
    QStringList getOneBranchCircuitEnergyKeys();


private:
    struct MyLastTariffCommand
    {
        quint16 startcommand;
        quint16 lastcommand;


//        ModbusAnswerList listMeterMessage;

        MyLastTariffCommand() {}
    } lasttariffcommand;

};

#endif // INTELIATSPWRDECODER_H
