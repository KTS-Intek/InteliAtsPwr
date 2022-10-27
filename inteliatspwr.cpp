#include "inteliatspwr.h"

///[!] type-converter
#include "src/base/convertatype.h"

#include "myucdevicetypes.h"


#define PLG_4_PC         1 //Parametrization
#define MAX_PLG_PREC    6



//------------------------------------------------------------------------------------------------------------

QString InteliAtsPwr::getMeterType()
{
    return QString("InteliATS-PWR");
}

//------------------------------------------------------------------------------------------------------------

quint16 InteliAtsPwr::getPluginVersion()
{
    return PLG_VER_RELEASE;
}

//------------------------------------------------------------------------------------------------------------

QString InteliAtsPwr::getMeterAddrsAndPsswdRegExp()
{
    return QString("%1%2").arg("^(0|[1-9]|[1-9][0-9]|1[0-9]{2}|2[0-3][0-9]|24[0-7])$").arg("^([0-9]{4})$");
}

//------------------------------------------------------------------------------------------------------------

quint8 InteliAtsPwr::getPasswdType()
{
    return UCM_PSWRD_DIGIT;
}

//------------------------------------------------------------------------------------------------------------

QString InteliAtsPwr::getVersion()
{
    return QString("InteliAtsPwr v0.0.2 %1").arg(QString(BUILDDATE));
}

//------------------------------------------------------------------------------------------------------------

QByteArray InteliAtsPwr::getDefPasswd()
{
    return InteliAtsPwrDecoder::getDefPasswd();
}

//------------------------------------------------------------------------------------------------------------

QString InteliAtsPwr::getSupportedMeterList()
{
    return QString("InteliATS-PWR");
}

//------------------------------------------------------------------------------------------------------------

quint8 InteliAtsPwr::getMaxTariffNumber(const QString &version)
{
    Q_UNUSED(version); return 1;
}

//------------------------------------------------------------------------------------------------------------

QStringList InteliAtsPwr::getEnrgList4thisMeter(const quint8 &pollCode, const QString &version)
{
    Q_UNUSED(version);
    return InteliAtsPwrDecoder::getSupportedEnrg(pollCode);
}

//------------------------------------------------------------------------------------------------------------

quint8 InteliAtsPwr::getMeterTypeTag()
{
    return  UC_METER_ELECTRICITY;
}

//------------------------------------------------------------------------------------------------------------

Mess2meterRezult InteliAtsPwr::mess2meter(const Mess2meterArgs &pairAboutMeter)
{
    return encoderdecoder.message2meter(pairAboutMeter);
}

//------------------------------------------------------------------------------------------------------------

QVariantHash InteliAtsPwr::decodeMeterData(const DecodeMeterMess &threeHash)
{
    return encoderdecoder.decodeMeterData(threeHash);
}

//------------------------------------------------------------------------------------------------------------

QVariantHash InteliAtsPwr::helloMeter(const QVariantHash &hashMeterConstData)
{
    Q_UNUSED(hashMeterConstData);
    QVariantHash hash;
    /*
    * bool data7EPt = hashMessage.value("data7EPt", false).toBool();
    * QByteArray endSymb = hashMessage.value("endSymb", QByteArray("\r\n")).toByteArray();
    * QByteArray currNI = hashMeterConstData.value("NI").toByteArray();
    * hashMessage.value("message")
*/
//    hash.insert("message", encoderdecoder.getReadMessage(hashMeterConstData, MODBUS_PMAC201HW_STARTTOTAL_REGISTER, MODBUS_PMAC201HW_TOTAL_FIRSTSTEP));
//    encoderdecoder.insertDefaultHashMessageValues(hash);//hash must be not empty
    return hash;
}

//------------------------------------------------------------------------------------------------------------

QString InteliAtsPwr::meterTypeFromMessage(const QByteArray &readArr)
{
    //do not detect, it is dangerous, coz many types of meters support modbus. And this one doesn't send its own type or model
    Q_UNUSED(readArr);
//    quint8 errcode;
//    if(ModbusProcessor4pmac201hw::isReceivedMessageValidFastCheck(ConvertAtype::convertArray2uint8list(readArr), encoderdecoder.lastAddr, errcode))
//        return getMeterType();

    return "";
}

//------------------------------------------------------------------------------------------------------------

QVariantHash InteliAtsPwr::isItYour(const QByteArray &arr)
{
    QByteArray dest;
    const MessageValidatorResult decoderesult = ModbusMessanger::messageIsValidExt(arr, encoderdecoder.lastAddr);
    if(decoderesult.isValid){
        dest = QByteArray::number(encoderdecoder.lastAddr);//is it good???
        QVariantHash hash;
        hash.insert("nodeID", dest + "\r\n");
        encoderdecoder.insertDefaultHashMessageValues(hash);
        return hash;
    }
    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------------

QVariantHash InteliAtsPwr::isItYourRead(const QByteArray &arr)
{
    //address (if it exists
    //crc, function codes

    //    quint8 addrs;
    ModbusDecodedParams messageparams;
    if((arr == encoderdecoder.lastGoodArr) ||
            (ModbusMessanger::isThisMessageYoursLoop(arr, messageparams) && encoderdecoder.lastAddr < 0xFF && encoderdecoder.lastAddr == messageparams.devaddress)){
        encoderdecoder.lastGoodArr = arr;
        QVariantHash hash;
        hash.insert("Tak", true);
        return hash;
    }

    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------------

QByteArray InteliAtsPwr::niChanged(const QByteArray &arr)
{
    Q_UNUSED(arr);
    return QByteArray();
}

//------------------------------------------------------------------------------------------------------------

QVariantHash InteliAtsPwr::meterSn2NI(const QString &meterSn)
{
    /*
         * hard - без варіантів (жорстко), від старого до нового ф-ту
         * altr - альтернатива для стандартного варіанту
         * <keys> : <QStringList>
         */
        //serial number from the NI   grp<x>_<modbus_address>

        QVariantHash h;

        if(meterSn.contains("_")){
            bool ok ;
            QString tmpStr = QString::number(QString(meterSn.split("_").last()).toULongLong(&ok), 16);
            const quint16 addrs = tmpStr.toUShort(&ok);
            if(ok && addrs > 0 && addrs < 248){
                h.insert("hard", QString::number(addrs));//
                return h;
            }
        }
        return h;
}

//------------------------------------------------------------------------------------------------------------

Mess2meterRezult InteliAtsPwr::messParamPamPam(const Mess2meterArgs &pairAboutMeter)
{
    Q_UNUSED(pairAboutMeter);
    return Mess2meterRezult();
}

//------------------------------------------------------------------------------------------------------------

QVariantHash InteliAtsPwr::decodeParamPamPam(const DecodeMeterMess &threeHash)
{
    Q_UNUSED(threeHash);
    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------------

QVariantHash InteliAtsPwr::how2logout(const QVariantHash &hashConstData, const QVariantHash &hashTmpData)
{
    Q_UNUSED(hashConstData);
    Q_UNUSED(hashTmpData);
    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------------

QVariantHash InteliAtsPwr::getDoNotSleep(const quint8 &minutes)
{
    Q_UNUSED(minutes);
    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------------

QVariantHash InteliAtsPwr::getGoToSleep(const quint8 &seconds)
{
    Q_UNUSED(seconds);
    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------------

QVariantList InteliAtsPwr::getDefaultVirtualMetersSchema()
{
    //it is used to create the default virtual meter table,
    // hashTmpData.insert("virtualmetersonly", true);
    QVariantList vmlist;
    const QStringList phaselists = QString("A B C").split(" ", QString::SkipEmptyParts);
    //Main
    //Generator
    for(int coilindx = 1, phaselmax = phaselists.size(), vmcounter = 1; coilindx <= 6; vmcounter++){
        QString onevirtualmeter;
        for(int phaseindx = 0; phaseindx < phaselmax; phaseindx++, coilindx++){
            const QString coilindxstr = QString::number(coilindx);
            onevirtualmeter.append(coilindxstr.rightJustified(3, '0'));
            onevirtualmeter.append(phaselists.at(phaseindx));
        }
        QVariantMap map;
        map.insert("GSN", onevirtualmeter);
        map.insert("memo", QString("virtual meter %1").arg(vmcounter));
        vmlist.append(map);
    }
    return vmlist;
}

//------------------------------------------------------------------------------------------------------------
