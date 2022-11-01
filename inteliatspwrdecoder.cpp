#include "inteliatspwrdecoder.h"


///[!] type-converter
#include "src/base/prettyvalues.h"
#include "src/base/convertatype.h"

#include "definedpollcodes.h"


//---------------------------------------------------------------------

InteliAtsPwrDecoder::InteliAtsPwrDecoder(QObject *parent) : QObject(parent)
{
    verboseMode = false;
    lastAddr = 0xFF;
}

//---------------------------------------------------------------------

QByteArray InteliAtsPwrDecoder::getDefPasswd()
{
    return QByteArray("");
}

//---------------------------------------------------------------------

QStringList InteliAtsPwrDecoder::getSupportedEnrg(const quint8 &code)
{
    if(code == POLL_CODE_READ_METER_STATE)
        return QString("relay").split(" ", QString::SkipEmptyParts);//relay state

    const QStringList enrgs = (code == POLL_CODE_READ_VOLTAGE) ?
                QString("UA,UB,UC,IA,IB,IC,PA,PB,PC,QA,QB,QC,cos_fA,cos_fB,cos_fC,F").split(',') :
                QString("A+;R-").split(";");

    return enrgs;
}

//---------------------------------------------------------------------

bool InteliAtsPwrDecoder::isPollCodeSupported(const quint16 &pollCode, QVariantHash &hashTmpData)
{
    if(hashTmpData.value("plgChecked").toBool())
        return true;

    const bool isSupp = isPollCodeSupported(pollCode);

    hashTmpData.insert("sprtdVls", isSupp);
    hashTmpData.insert("plgChecked", true);
    return isSupp;
}

//---------------------------------------------------------------------

bool InteliAtsPwrDecoder::isPollCodeSupported(const quint16 &pollCode)
{
    return (pollCode == POLL_CODE_READ_VOLTAGE || pollCode == POLL_CODE_READ_TOTAL ||
            pollCode == POLL_CODE_READ_METER_STATE);

}

//---------------------------------------------------------------------

QByteArray InteliAtsPwrDecoder::getMyModbusReadRegisterMessage(const quint8 &address, const quint16 &registerno, const quint16 &len)
{
    return ModbusMessanger::getModbusReadRegisterMessage(address, registerno - MODBUS_IANTPWR_MINUS_REGISTER, len);

}

//---------------------------------------------------------------------

QByteArray InteliAtsPwrDecoder::meterNiFromConstHash(const QVariantHash &hashConstData)
{
    return hashConstData.value("NI").toByteArray();
}

//---------------------------------------------------------------------

QByteArray InteliAtsPwrDecoder::getReadMessage(const QByteArray &ni, const quint16 &startcommand, const quint16 &lastcommand)
{
    bool ok;
    const quint16 address = ni.toUShort(&ok);
    if( address < 1 || (address > 247)  || lastcommand < startcommand || !ok)
        return QByteArray();//bad address

    lasttariffcommand.lastcommand = lastcommand;
    lasttariffcommand.startcommand = startcommand;

    lastAddr = address;
    return getMyModbusReadRegisterMessage(address, startcommand, lastcommand - startcommand );//+1
}

//---------------------------------------------------------------------

QByteArray InteliAtsPwrDecoder::getReadMessage(const QVariantHash &hashConstData, const quint16 &startcommand, const quint16 &lastcommand)
{
    return getReadMessage(meterNiFromConstHash(hashConstData), startcommand, lastcommand);

}

//---------------------------------------------------------------------

QVariantHash InteliAtsPwrDecoder::decodeEnd(const QVariantHash &decodehash, const ErrCounters &errwarns, quint16 &step, QVariantHash &hashTmpData)
{
    hashTmpData.insert("virtualmetersonly", true);

    if(!decodehash.isEmpty()){
        MeterPluginHelper::copyHash2hash(decodehash, hashTmpData);

        if(!hashTmpData.value("messFail").toBool() && hashTmpData.value("currEnrg", 0).toInt() > 3){
            hashTmpData.insert("currEnrg", 5);
            step = 0xFFFF;
        }
    }

    if(verboseMode && errwarns.error_counter > 0){
        const ErrCounters errwarnsmirror   = ErrCounters(qMax(0, hashTmpData.value("warning_counter", 0).toInt()), qMax(0, hashTmpData.value("error_counter", 0).toInt()));
        if(errwarns.error_counter > errwarnsmirror.error_counter){
            for(int i = 0; i < errwarns.error_counter && verboseMode; i++)
                qDebug() << "InteliAtsPwrDecoder error " << hashTmpData.value(QString("Error_%1").arg(i));
        }


    }
    hashTmpData.insert("step", step);
    hashTmpData.insert("error_counter", errwarns.error_counter);
    hashTmpData.insert("warning_counter", errwarns.warning_counter);

    return hashTmpData;
}

//---------------------------------------------------------------------

Mess2meterRezult InteliAtsPwrDecoder::message2meter(const Mess2meterArgs &pairAboutMeter)
{
    plgtimeg.start();

    const QVariantHash hashConstData = pairAboutMeter.hashConstData;
    QVariantHash hashTmpData = pairAboutMeter.hashTmpData;
    QVariantHash hashMessage;

    const quint8 pollCode = hashConstData.value("pollCode").toUInt();
    if(!isPollCodeSupported(pollCode, hashTmpData))
        return Mess2meterRezult(hashMessage,hashTmpData);


    verboseMode = hashConstData.value("verboseMode").toBool();


    quint16 step = hashTmpData.value("step", (quint16)0).toUInt();

    if(verboseMode) qDebug() << "mess2Meter " << step << pollCode ;



    switch (pollCode) {
    case POLL_CODE_READ_VOLTAGE         : hashMessage = preparyVoltage(hashConstData, hashTmpData)          ; break;
    case POLL_CODE_READ_TOTAL           : hashMessage = preparyTotalEnrg(hashConstData, hashTmpData)        ; break;
    case POLL_CODE_READ_METER_STATE     : hashMessage = preparyMeterState(hashConstData, hashTmpData)        ; break;

    default:{ if(verboseMode) qDebug() << "PMAC201HW pollCode is not supported 1" << pollCode                  ; break;}
    }


    if(hashMessage.isEmpty())
        step = 0xFFFF;

    hashTmpData.insert("step", step);
    if(step >= 0xFFFE)
        hashMessage.clear();
    else
        insertDefaultHashMessageValues(hashMessage);


    return Mess2meterRezult(hashMessage,hashTmpData);
}

//---------------------------------------------------------------------

QVariantHash InteliAtsPwrDecoder::decodeMeterData(const DecodeMeterMess &threeHash)
{
    const QVariantHash hashConstData = threeHash.hashConstData;
    const QVariantHash hashRead = threeHash.hashRead;
    QVariantHash hashTmpData = threeHash.hashTmpData;


    if(verboseMode){
        foreach (QString key, hashRead.keys()) {
            qDebug() << "PMAC201HW read "  << key << hashRead.value(key).toByteArray().toHex().toUpper();
        }
    }

    if(!hashTmpData.contains("hasnosn"))
        hashTmpData.insert("hasnosn", true);

    hashTmpData.insert("messFail", true);
    quint8 pollCode         = hashConstData.value("pollCode").toUInt();
    quint16 step            = hashTmpData.value("step", (quint16)0).toUInt();
    ErrCounters errwarns   = ErrCounters(qMax(0, hashTmpData.value("warning_counter", 0).toInt()), qMax(0, hashTmpData.value("error_counter", 0).toInt()));


    const MessageValidatorResult decoderesult = ModbusMessanger::messageIsValidExt(hashRead.value("readArr_0").toByteArray(), lastAddr);
    if(!decoderesult.isValid){
        QString warn;
        QString err = decoderesult.errstr;
        if(hashRead.value("readArr_0").toByteArray().isEmpty()) //it must have some data
            hashTmpData.insert(MeterPluginHelper::errWarnKey(errwarns.error_counter, true),
                               MeterPluginHelper::prettyMess(tr("incomming data is invalid"),
                                                             PrettyValues::prettyHexDump( hashRead.value("readArr_0").toByteArray().toHex(), "", 0)
                                                             , err, warn, true));
        pollCode = 0;
    }

    if(hashTmpData.value("SN").toString().isEmpty() && pollCode > 0){
        hashTmpData.insert("SN", QString::number(lastAddr));

        //        hashTmpData.insert("hasVirtualMeters", true);
        //hashTmpData.insert("")
    }


    QVariantHash decodehash;
    switch(pollCode){
    case POLL_CODE_READ_VOLTAGE         : decodehash = fullVoltage(  decoderesult, hashTmpData, step, errwarns);break;
    case POLL_CODE_READ_TOTAL           : decodehash = fullTotalEnrg(decoderesult, hashTmpData, step, errwarns); break;
    case POLL_CODE_READ_METER_STATE     : decodehash = fullMeterState(decoderesult, hashTmpData, step, errwarns); break;

    }

    if(!decodehash.value("messFail").toBool() && !hashTmpData.contains("SN")){
        decodehash.insert("SN", hashConstData.value("ModemNI").toString());
    }

    return decodeEnd(decodehash, errwarns, step, hashTmpData);
}

//---------------------------------------------------------------------

QVariantHash InteliAtsPwrDecoder::preparyVoltage(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{
    Q_UNUSED(hashTmpData);

    QVariantHash hashMessage;

    quint16 messagecommand = MODBUS_IANTPWR_VOLTAGE_STEP1;
    quint16 lastcommand = messagecommand + MODBUS_IANTPWR_VOLTAGE_FIRSTSTEP;

    hashMessage.insert("message_0", getReadMessage(hashConstData,messagecommand, lastcommand));

    return hashMessage;
}

//---------------------------------------------------------------------

QVariantHash InteliAtsPwrDecoder::fullVoltage(const MessageValidatorResult &decoderesult, const QVariantHash &hashTmpData, quint16 &step, ErrCounters &warnerr)
{
    Q_UNUSED(warnerr);
    Q_UNUSED(hashTmpData);
    QVariantHash resulthash;

    if(decoderesult.listMeterMessage.isEmpty() || decoderesult.errCode != MODBUS_ERROR_HAS_NO_ERRORS || decoderesult.listMeterMessage.size() < 70){
        resulthash.insert("messFail", true);
        resulthash.insert("IANTPWR_step", 0);
        return resulthash;
    }

    /*
 * 1. get vrsn
 * 2. what CB is Closed MCB or GCB? It has only one set of CTs, so apply Load values to the active CB
 * 3. gen and mains freq
 * 4. generate vm_x with phase A values
*/


    //add vrsn
    addDevVersion(resulthash, decoderesult.listMeterMessage);


    const QHash<QString, QString> voltageLoadFreqValues = addVoltageLoadFreqValues(decoderesult.listMeterMessage);
    if(voltageLoadFreqValues.isEmpty()){
        resulthash.insert("messFail", true);
        resulthash.insert("IANTPWR_step", 0);
        return resulthash;
    }

    const bool ledGCBGreen = decoderesult.listMeterMessage.at(64); //40065 generator CB
    const bool ledMCBGreen = decoderesult.listMeterMessage.at(65); //40066 mains CB



    //   const QStringList lkeys = QString("GUA,GUB,GUC,40004,40005,"
    //                                     "6,7,IA,IB,IC,"
    //                                     "11,GF,13,14,PA,"
    //                                     "PB,PC,18,19,QA,"
    //                                     "QB,QC,23,cos_fA,cos_fB"
    //                                     "cos_fC,27,28,29,30,"
    //                                     "31,32,33,34,35,"
    //                                     "MUA,MUB,MUC,39,40,"
    //                                     "41,42,MF");

    quint16 vmIndex = 0U;// = hashTmpData.value("lastVirtualMeterIndex", 0U).toUInt();

    const QStringList listUKeys = QString("UA,UB,UC").split(",");//,F
    const QStringList listLoadKeys = QString("IA,IB,IC,"
                                             "PA,PB,PC,"
                                             "QA,QB,QC,"
                                             "cos_fA,cos_fB,cos_fC").split(",");


    //it has a pair of 3phases
    // Generator and Main

    /*
     * One IA-NT has two meters Gen and Mains. Both are 3ph.
     * vm_1, vm_2, vm_3  are Gen L1, L2, L3 (or A, B, C) sensors
     * vm_4, vm_5, vm_6  are Mains L1, L2, L3 (or A, B, C) sensors
    */
    for(int c = 0, lkMax = listLoadKeys.size(); c < 2; c++){
        const QString prefix = (c == 0) ? "G" : "M";

        for(int i = 0; i < 3; i++){
            const QString vmkey = QString("vm_%1").arg(vmIndex + 1);

            QVariantHash onevmhash;

            onevmhash.insert("UX", voltageLoadFreqValues.value(prefix + listUKeys.at(i)));//add A,B,C values as A value
            onevmhash.insert("F", voltageLoadFreqValues.value(prefix + "F"));

            const QString enrgPrefix = ((c == 0 && ledGCBGreen) || (c == 1 && ledMCBGreen)) ? "" : "ignore";

            //current is from generator
            //current is from mains

            for(int j = i; j < lkMax; j += 3){
                //so if c is 0 and ledGCBGreen or c is 1 and ledMCBGreen  it fills with a real values, in other case it fills it with 0 values
                QString outKey = listLoadKeys.at(j) + enrgPrefix;
                const QString srcKey = outKey;
                outKey.chop(1);
                outKey.append("X");//use X the add2database method adds it with necessary letter

                onevmhash.insert(outKey, voltageLoadFreqValues.value(srcKey, "0"));
            }

            resulthash.insert(vmkey, onevmhash);//it starts from 1
            vmIndex++;
        }


    }


    if(verboseMode)
        qDebug() << "InteliAtsPwrDecoder::fullVoltage resulthash " << resulthash ;

    resulthash.insert("messFail", false);//go to the next energy
    resulthash.insert("lastVirtualMeterIndex", vmIndex);//go to the next virtual meter

    step = 0xFFFF;

    return resulthash;
}

//---------------------------------------------------------------------

QVariantHash InteliAtsPwrDecoder::preparyTotalEnrg(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{
    if(hashTmpData.value("vrsn").toString().isEmpty()){
        if(hashConstData.value("vrsn").toString().isEmpty())
            return preparyVoltage(hashConstData, hashTmpData);

        hashTmpData.insert("vrsn", hashConstData.value("vrsn"));
    }



    const quint8 localstep = hashTmpData.value("IANT_step", 0).toUInt();

    //    43006-43007 ( 2) 8205     Genset kWh          Integer     4   0      -      - Statistics
    //    43008-43009 ( 2) 8539     Genset kVArh        Integer     4   0      -      - Statistics

    //    43201-43202 ( 2) 11025    Mains kWh           Integer     4   0      -      - Statistics
    //    43203-43204 ( 2) 11026    Mains kVArh         Integer     4   0      -      - Statistics

    QVariantHash hashMessage;
    quint16 messagecommand, lastcommand;

    //A+,R-
    messagecommand = (localstep == 1) ? MODBUS_IANTPWR_LASTTOTAL_REGISTER : MODBUS_IANTPWR_STARTTOTAL_REGISTER;
    lastcommand = messagecommand + MODBUS_IANTPWR_TOTAL_STEP;

    //#define MODBUS_IANTPWR_STARTTOTAL_REGISTER    43006 //43006-43007 ( 2) 8205     Genset kWh  43008-43009 ( 2) 8539     Genset kVArh
    //#define MODBUS_IANTPWR_LASTTOTAL_REGISTER     43201 //43201-43202 ( 2) 11025    Mains kWh  43203-43204 ( 2) 11026    Mains kVArh
    //#define MODBUS_IANTPWR_TOTAL_STEP 4 //a pair of kwh and kvar

    hashMessage.insert("message_0", getReadMessage(hashConstData,messagecommand, lastcommand));


    return hashMessage;
}

//---------------------------------------------------------------------

QVariantHash InteliAtsPwrDecoder::fullTotalEnrg(const MessageValidatorResult &decoderesult, const QVariantHash &hashTmpData, quint16 &step, ErrCounters &warnerr)
{
    Q_UNUSED(warnerr);
    Q_UNUSED(hashTmpData);

    QVariantHash resulthash;

    if(decoderesult.listMeterMessage.isEmpty() || decoderesult.errCode != MODBUS_ERROR_HAS_NO_ERRORS || decoderesult.listMeterMessage.length() < 4){
        resulthash.insert("messFail", true);
        return resulthash;
    }


    if(hashTmpData.value("vrsn").toString().isEmpty()){
        //receve vrsn

        if(decoderesult.listMeterMessage.size() < 70){
            resulthash.insert("messFail", true);
            return resulthash;
        }
        //add vrsn
        addDevVersion(resulthash, decoderesult.listMeterMessage);
        resulthash.insert("messFail", false);//go to the next step
        return resulthash;
    }
    const quint8 localstep = hashTmpData.value("IANT_step", 0).toUInt();


    quint16 vmIndex = (localstep == 1) ? 3 : 0;//hashTmpData.value("lastVirtualMeterIndex", 0U).toUInt();

    //it has a pair of 3phases
    // Generator and Main

    /*
     * One IA-NT has two meters Gen and Mains. Both are 3ph.
     * vm_1, vm_2, vm_3  are Gen kWh,0,0 and kvar,0,0
     * vm_4, vm_5, vm_6  are Mains kWh,0,0 and kvar,0,0
    */

    const QStringList enrgk = getOneBranchCircuitEnergyKeys();
    const QStringList ldata = getOneBranchCircuitEnergy(decoderesult.listMeterMessage.mid(0,4));

    //    if(localstep == 0){ //A+, A-
    //        vmIndex = 0U;
    //    }//else R+,R-

   const QStringList ldataz = QString("0,0,0,0").split(",");

    for(int i = 0; i < 3; i++){
        const QString vmkey = QString("vm_%1").arg(vmIndex + 1);
        QVariantHash onevmhash;
        insertLists2hash(onevmhash, enrgk, (i == 0) ? ldata : ldataz);
        resulthash.insert(vmkey, onevmhash);//it starts from 1
        vmIndex++;
        if(verboseMode)
            qDebug() << "InteliAtsPwrDecoder::fullTotalEnrg " << i << ldata << enrgk;
    }


    if(verboseMode)
        qDebug() << "InteliAtsPwrDecoder::fullTotalEnrg resulthash " << resulthash ;

    resulthash.insert("messFail", false);//go to the next energy
    resulthash.insert("lastVirtualMeterIndex", vmIndex);//go to the next virtual meter
    resulthash.insert("IANT_step", localstep + 1);

    if(localstep > 0)
        step = 0xFFFF;

    return resulthash;
}

//---------------------------------------------------------------------

QVariantHash InteliAtsPwrDecoder::preparyMeterState(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{
    //all necessary registers are received  with voltage values
    return preparyVoltage(hashConstData, hashTmpData);

}

//---------------------------------------------------------------------

QVariantHash InteliAtsPwrDecoder::fullMeterState(const MessageValidatorResult &decoderesult, const QVariantHash &hashTmpData, quint16 &step, ErrCounters &warnerr)
{
    Q_UNUSED(warnerr);
    Q_UNUSED(hashTmpData);
    QVariantHash resulthash;

    if(decoderesult.listMeterMessage.isEmpty() || decoderesult.errCode != MODBUS_ERROR_HAS_NO_ERRORS || decoderesult.listMeterMessage.size() < 70){
        resulthash.insert("messFail", true);
        resulthash.insert("IANTPWR_step", 0);
        return resulthash;
    }

    /*
 * 1. get vrsn
 * 2. what CB is Closed MCB or GCB? It has only one set of CTs, so apply Load values to the active CB
 * 3. gen and mains freq
 * 4. generate vm_x with phase A values
*/


    //add vrsn
    addDevVersion(resulthash, decoderesult.listMeterMessage);

    const bool ledGCBGreen = decoderesult.listMeterMessage.at(64); //40065 generator CB
    const bool ledMCBGreen = decoderesult.listMeterMessage.at(65); //40066 mains CB


    quint16 vmIndex = 0U;// = hashTmpData.value("lastVirtualMeterIndex", 0U).toUInt();


    //it has a pair of 3phases
    // Generator and Main

    /*
     * One IA-NT has two meters Gen and Mains. Both are 3ph.
     * vm_1, vm_2, vm_3  are Gen L1, L2, L3 (or A, B, C) sensors
     * vm_4, vm_5, vm_6  are Mains L1, L2, L3 (or A, B, C) sensors
    */


    for(int i = 0; i < 6; i++){


//#define RELAY_STATE_UNKN        0
//#define RELAY_STATE_LOAD_ON     1  //main "relay_all_power_on" : secondary "relay_2_power_on"
//#define RELAY_STATE_LOAD_OFF    2
//#define RELAY_STATE_NOT_SUP     3
//#define RELAY_STATE_OFF_PRSBTTN 4

        const QString vmkey = QString("vm_%1").arg(vmIndex + 1);

        QVariantHash onevmhash;
        quint8 mainstts = RELAY_STATE_UNKN;

        mainstts = ((i < 3 && ledGCBGreen) || (i >= 3 && ledMCBGreen)) ? RELAY_STATE_LOAD_ON : RELAY_STATE_LOAD_OFF;

        MeterPluginHelper::addRelayStatusAll(onevmhash, mainstts, RELAY_STATE_NOT_SUP);
        resulthash.insert(vmkey, onevmhash);//it starts from 1
        vmIndex++;

    }


    if(verboseMode)
        qDebug() << "InteliAtsPwrDecoder::fullMeterState resulthash " << resulthash ;

    resulthash.insert("messFail", false);//go to the next energy
    resulthash.insert("lastVirtualMeterIndex", vmIndex);//go to the next virtual meter

    step = 0xFFFF;

    return resulthash;
}

//---------------------------------------------------------------------

void InteliAtsPwrDecoder::insertDefaultHashMessageValues(QVariantHash &hashMessage)
{
    if(hashMessage.isEmpty())
        return;//there are no messages
    hashMessage.insert("endSymb", "");
    hashMessage.insert("quickCRC", true);
    hashMessage.insert("data7EPt", false);
    hashMessage.insert("readLen", MODBUS_MIN_READLEN);
}

//---------------------------------------------------------------------

void InteliAtsPwrDecoder::insertLists2hash(QVariantHash &hash, const QStringList &lkeys, const QStringList &ldata)
{
    const int imax = lkeys.size();
    const int imax2 = ldata.size();

    if(imax != imax2)
        qDebug() << "bad len";

    for(int i = 0; i < imax; i++)
        hash.insert(lkeys.at(i), (i < imax2) ? ldata.at(i) : "-");
}



//---------------------------------------------------------------------

void InteliAtsPwrDecoder::addDevVersion(QVariantHash &hashTmpData, const ModbusAnswerList &listMeterMessage)
{

    //    40075            8707     FW Branch           Unsigned    1   0      -      - IA Info
    //    40076            8393     FW Version          Unsigned    1   1      -      - IA Info
    //    40077            8480     Application         Unsigned    1   0      -      - IA Info
    QStringList vrsn;

    for(int i = 40075 - MODBUS_IANTPWR_STARTVOLTAGE_REGISTER, imax = listMeterMessage.size(), j = 0; i < imax && j < 3; i++, j++){
        vrsn.prepend(QString::number(listMeterMessage.at(i)));
    }

    if(vrsn.isEmpty())
        return;
    hashTmpData.insert("vrsn", QString("FW.%1").arg(vrsn.join(".")));

    //    const QList<int> lindex = QList<int>() << 0 << 1 << 2 << 10 :

}

//---------------------------------------------------------------------

QHash<QString, QString> InteliAtsPwrDecoder::addVoltageLoadFreqValues(const ModbusAnswerList &listMeterMessage)
{
    //    40001            8192     Gen V L1-N     V    Unsigned    2   0      -      - Generator
    //    40002            8193     Gen V L2-N     V    Unsigned    2   0      -      - Generator
    //    40003            8194     Gen V L3-N     V    Unsigned    2   0      -      - Generator
    //    40004            10645    (N/A)
    //    40005            9628     Gen V L1-L2    V    Unsigned    2   0      -      - Generator

    //    40006            9629     Gen V L2-L3    V    Unsigned    2   0      -      - Generator
    //    40007            9630     Gen V L3-L1    V    Unsigned    2   0      -      - Generator
    //    40008            8198     Load A L1      A    Unsigned    2   0      -      - Load
    //    40009            8199     Load A L2      A    Unsigned    2   0      -      - Load
    //    40010            8200     Load A L3      A    Unsigned    2   0      -      - Load

    //    40011            8209     (N/A)
    //    40012            8210     Gen Freq       Hz   Unsigned    2   1      -      - Generator
    //    40013            10899    (N/A)
    //    40014            8202     Load kW        kW   Integer     2   0      -      - Load
    //    40015            8524     Load kW L1     kW   Integer     2   0      -      - Load

    //    40016            8525     Load kW L2     kW   Integer     2   0      -      - Load
    //    40017            8526     Load kW L3     kW   Integer     2   0      -      - Load
    //    40018            9018     Nominal kVA    kVA  Integer     2   0      -      - Invisible
    //    40019            8203     Load kVAr      kVAr Integer     2   0      -      - Load
    //    40020            8527     Load kVAr L1   kVAr Integer     2   0      -      - Load

    //    40021            8528     Load kVAr L2   kVAr Integer     2   0      -      - Load
    //    40022            8529     Load kVAr L3   kVAr Integer     2   0      -      - Load
    //    40023            8204     Load PF             Integer     1   2      -      - Load
    //    40024            8533     Load PF L1          Integer     1   2      -      - Load
    //    40025            8534     Load PF L2          Integer     1   2      -      - Load

    //    40026            8535     Load PF L3          Integer     1   2      -      - Load
    //    40027            8226     (N/A)
    //    40028            8395     Load Char           Char        1   -      -      - Load
    //    40029            8626     Load Char L1        Char        1   -      -      - Load
    //    40030            8627     Load Char L2        Char        1   -      -      - Load

    //    40031            8628     Load Char L3        Char        1   -      -      - Load
    //    40032            8565     Load kVA       kVA  Integer     2   0      -      - Load
    //    40033            8530     Load kVA L1    kVA  Integer     2   0      -      - Load
    //    40034            8531     Load kVA L2    kVA  Integer     2   0      -      - Load
    //    40035            8532     Load kVA L3    kVA  Integer     2   0      -      - Load

    //    40036            8195     Mains V L1-N   V    Unsigned    2   0      -      - Mains
    //    40037            8196     Mains V L2-N   V    Unsigned    2   0      -      - Mains
    //    40038            8197     Mains V L3-N   V    Unsigned    2   0      -      - Mains
    //    40039            10666    (N/A)
    //    40040            9631     Mains V L1-L2  V    Unsigned    2   0      -      - Mains

    //    40041            9632     Mains V L2-L3  V    Unsigned    2   0      -      - Mains
    //    40042            9633     Mains V L3-L1  V    Unsigned    2   0      -      - Mains
    //    40043            8211     Mains Freq     Hz   Unsigned    2   1      -      - Mains


    const QStringList lkeys = QString("GUA,GUB,GUC,40004,40005,"
                                      "6,7,IA,IB,IC,"
                                      "11,GF,13,14,PA,"
                                      "PB,PC,18,19,QA,"
                                      "QB,QC,23,cos_fA,cos_fB,"
                                      "cos_fC,27,28,29,30,"
                                      "31,32,33,34,35,"
                                      "MUA,MUB,MUC,39,40,"
                                      "41,42,MF").split(",");

    QMap<QString,qreal> mapDecimals;
    mapDecimals.insert("GF", 0.1);
    mapDecimals.insert("MF", 0.1);

    mapDecimals.insert("cos_fA", 0.01);
    mapDecimals.insert("cos_fB", 0.01);
    mapDecimals.insert("cos_fC", 0.01);


//    QMap<QString,bool> mapSignedValues;

//    mapSignedValues.insert("P", true);
//    mapSignedValues.insert("Q", true);
//    mapSignedValues.insert("cos_f", true);





    const int imax = lkeys.size();

    QHash<QString, QString> out;
    if(listMeterMessage.size() < imax)
        return out;

    for(int i = 0; i < imax; i++){
        const QString key = lkeys.at(i);
        const bool isSigned = (key.startsWith("P") || key.startsWith("Q") || key.startsWith("cos_f"));

        const qreal value = (isSigned ?
                                 qreal(qint16(listMeterMessage.at(i)))
                               : qreal(listMeterMessage.at(i)))
                * mapDecimals.value(key, 1.0);

        out.insert(key, PrettyValues::prettyNumber(value, 2, 2));
    }
    return out;

}




//---------------------------------------------------------------------

QStringList InteliAtsPwrDecoder::getOneBranchCircuitEnergy(const ModbusAnswerList &listMeterMessage)
{
    QStringList out;
    if(listMeterMessage.size() == 4){

        out = ModbusMessanger::convertTwoRegisters2oneValueStrExt(listMeterMessage, 1.0, 1, true);
        out.append(out.last());//It has only one rate, x- add to the end (x is A- or R-
        out.prepend(out.first()); //because the minimum data is Summ and T1 , x+ add to the beginning (x+ is A+ or R+)
    }
    if(verboseMode)
        qDebug() << "InteliAtsPwrDecoder::getOneBranchCircuitEnergy " << out;
    return out;
}

//---------------------------------------------------------------------

QStringList InteliAtsPwrDecoder::getOneBranchCircuitEnergyKeys()
{
    //    return QString("T0_A+,T1_A+,T0_R-,T1_R-").split(",");

    return QString("T0_A+,T1_A+,T0_R-,T1_R-").split(",");
}

//---------------------------------------------------------------------
