#ifndef INTELIATSPWR_H
#define INTELIATSPWR_H


#include <QObject>



#include <QtPlugin>
#include <QPair>

#include "shared/meterplugin.h"
#include "inteliatspwrdecoder.h"


class InteliAtsPwr : public QObject, MeterPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "ua.zbyralko.hello_zb.MeterPlugin" FILE "zbyralko.json")
    Q_INTERFACES(MeterPlugin)

public:
    QString getMeterType() ;

    quint16 getPluginVersion() ;

    QString getMeterAddrsAndPsswdRegExp();

    quint8 getPasswdType() ;

    QString getVersion() ;

    QByteArray getDefPasswd() ;

    QString getSupportedMeterList() ;

    quint8 getMaxTariffNumber(const QString &version) ;

    QStringList getEnrgList4thisMeter(const quint8 &pollCode, const QString &version) ;

    quint8 getMeterTypeTag();

    Mess2meterRezult mess2meter(const Mess2meterArgs &pairAboutMeter);

    QVariantHash decodeMeterData(const DecodeMeterMess &threeHash);

    QVariantHash helloMeter(const QVariantHash &hashMeterConstData);

    QString meterTypeFromMessage(const QByteArray &readArr);

    QVariantHash isItYour(const QByteArray &arr);

    QVariantHash isItYourRead(const QByteArray &arr);

    QByteArray niChanged(const QByteArray &arr);

    QVariantHash meterSn2NI(const QString &meterSn);



    //parametryzatsiya
    Mess2meterRezult messParamPamPam(const Mess2meterArgs &pairAboutMeter);
    QVariantHash decodeParamPamPam(const DecodeMeterMess &threeHash) ;

    QVariantHash how2logout(const QVariantHash &hashConstData, const QVariantHash &hashTmpData) ;

    QVariantHash getDoNotSleep(const quint8 &minutes);

    QVariantHash getGoToSleep(const quint8 &seconds) ;

    QVariantList getDefaultVirtualMetersSchema();

private:
    InteliAtsPwrDecoder encoderdecoder;
};

#endif // INTELIATSPWR_H
