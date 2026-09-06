#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QSharedPointer>
#include <QVariant>

class Settings
{
public:
    Settings(QString organization, QString application);

    QSettings qSettings;

    typedef QMap<QString, QVariant> KeyVals;
    typedef QList<KeyVals> KeyValList;

    struct Setting {
        Setting(Settings* parent, QString key);
        QVariant getOrDefault(QVariant defaultValue = QVariant());
        void set(QVariant value);
        QString key();
        bool isSet();
    private:
        Settings* mParent;
        QString mKey;
    };

    struct Array {
        Array(Settings* parent, QString key);
        KeyValList get();
        void set(KeyValList items);
    private:
        Settings* mParent;
        QString mKey;
    };

    typedef QSharedPointer<Setting> SettingPtr;
    typedef QSharedPointer<Array> ArrayPtr;

    SettingPtr newSetting(QString key);
    ArrayPtr newArray(QString key);

    bool contains(QString key);

    QVariant getOrDefault(QString key, QVariant defaultValue = QVariant());
    void set(QString key, QVariant value);

    QList<QMap<QString, QVariant> > getArray(QString arrayName);
    void setArray(QString arrayName, QList<QMap<QString, QVariant>> items);

    QMap<QString, QString> getGroupKeyVals(QString group);
    void setGroupKeyVals(QString group, const QMap<QString, QString> keyvals);
};

// =============================================================================

class SimpleSerialSettings : public Settings
{
public:
    SimpleSerialSettings();

    SettingPtr autoScroll                   = newSetting("autoScroll");
    SettingPtr crLf                         = newSetting("crlf");
    SettingPtr displayModeText              = newSetting("displayModeText");
    SettingPtr displayModeHex               = newSetting("displayModeHex");
    SettingPtr hexSpecial                   = newSetting("hexSpecial");
    SettingPtr showCrLfHex                  = newSetting("showCrLfHex");
    SettingPtr newlineForCrLf               = newSetting("newlineForCrLf");
    SettingPtr tabWidth                     = newSetting("tabWidth");
    SettingPtr showDuck                     = newSetting("showDuck");
    SettingPtr replaceEscapeSequences       = newSetting("replaceEscapeSequences");
    SettingPtr showSentData                 = newSetting("showSentData");
    SettingPtr sentDataOnSeparateLine       = newSetting("sentDataOnSeparateLine");
    SettingPtr tcpServerPort                = newSetting("tcpServerPort");
    SettingPtr tcpClientIp                  = newSetting("tcpClientIp");
    SettingPtr tcpClientPort                = newSetting("tcpClientPort");
    SettingPtr udpBindForListen             = newSetting("udpBindForListen");
    SettingPtr udpBindPort                  = newSetting("udpBindPort");
    SettingPtr udpSendBroadcast             = newSetting("udpSendBroadcast");
    SettingPtr udpSendIp                    = newSetting("udpSendIp");
    SettingPtr udpSendPort                  = newSetting("udpSendPort");
    SettingPtr sendFilePath                 = newSetting("sendFilePath");
    SettingPtr sendFileFrequencyMs          = newSetting("sendFileFrequencyMs");
    SettingPtr sendFileExcludeEndingNewline = newSetting("sendFileExcludeEndingNewline");
    SettingPtr sendFileSendMsgIfFileEmpty   = newSetting("sendFileSendMsgIfFileEmpty");
    SettingPtr sendFileMsgIfEmpty           = newSetting("sendFileMsgIfEmpty");
    SettingPtr timestampsEnabled            = newSetting("timestampsEnabled");
    SettingPtr timestampsOnlyAfterNewlines  = newSetting("timestampsOnlyAfterNewlines");
    SettingPtr timestampGroupTimeMs         = newSetting("timestampGroupTimeMs");
    SettingPtr timestampAddTabAfter         = newSetting("timestampAddTabAfter");
    SettingPtr macrosSendCrlf               = newSetting("macrosSendCrlf");
    ArrayPtr   macros                       = newArray("macros");
    SettingPtr macroValue                   = newSetting("macroValue");
    SettingPtr consoleFontPointSize         = newSetting("consoleFontPointSize");
    SettingPtr guiStyle                     = newSetting("guistyle");
    SettingPtr consoleFont                  = newSetting("consoleFont");
};

#endif // SETTINGS_H
