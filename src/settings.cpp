#include "settings.h"


Settings::Settings(QString organization, QString application)
    : qSettings(QSettings::NativeFormat, QSettings::UserScope,
                organization, application)
{

}

bool Settings::contains(QString key)
{
    return qSettings.contains(key);
}

QVariant Settings::getOrDefault(QString key, QVariant defaultValue)
{
    return qSettings.value(key, defaultValue);
}

void Settings::set(QString key, QVariant value)
{
    qSettings.setValue(key, value);
}

QList<QMap<QString, QVariant> > Settings::getArray(QString arrayName)
{
    QList<QMap<QString, QVariant>> items;
    int size = qSettings.beginReadArray(arrayName);
    for (int i = 0; i < size; i++) {
        qSettings.setArrayIndex(i);
        QStringList keys = qSettings.childKeys();
        QMap<QString, QVariant> keyVals;
        foreach (QString key, keys) {
            keyVals.insert(key, qSettings.value(key).toString());
        }
        items.append(keyVals);
    }
    qSettings.endArray();
    return items;
}

void Settings::setArray(QString arrayName, QList<QMap<QString, QVariant> > items)
{
    qSettings.beginWriteArray(arrayName);
    for (int i = 0; i < items.size(); i++) {
        qSettings.setArrayIndex(i);
        QMap<QString, QVariant> keyVals = items.value(i);
        foreach (QString key, keyVals.keys()) {
            qSettings.setValue(key, keyVals.value(key));
        }
    }
    qSettings.endArray();
}

QMap<QString, QString> Settings::getGroupKeyVals(QString group)
{
    QMap<QString, QString> keyvals;
    qSettings.beginGroup(group);
    foreach (QString key, qSettings.allKeys()) {
        keyvals.insert(key, qSettings.value(key).toString());
    }
    qSettings.endGroup();
    return keyvals;
}

void Settings::setGroupKeyVals(QString group, const QMap<QString, QString> keyvals)
{
    qSettings.beginGroup(group);
    for (const QString& key : keyvals.keys()) {
        qSettings.setValue(key, keyvals.value(key));
    }
    qSettings.endGroup();
}

Settings::ArrayPtr Settings::newArray(QString key)
{
    return ArrayPtr::create(this, key);
}

Settings::SettingPtr Settings::newSetting(QString key)
{
    return SettingPtr::create(this, key);
}

SimpleSerialSettings::SimpleSerialSettings()
    : Settings("Noedigcode", "SimpleSerial")
{

}

Settings::Setting::Setting(Settings *parent, QString key)
    : mParent(parent), mKey(key)
{
}

QVariant Settings::Setting::getOrDefault(QVariant defaultValue)
{
    return mParent->getOrDefault(mKey, defaultValue);
}

void Settings::Setting::set(QVariant value)
{
    mParent->set(mKey, value);
}

QString Settings::Setting::key()
{
    return mKey;
}

bool Settings::Setting::isSet()
{
    return mParent->contains(mKey);
}

Settings::Array::Array(Settings *parent, QString key)
    : mParent(parent), mKey(key)
{
}

void Settings::Array::set(KeyValList items)
{
    mParent->setArray(mKey, items);
}

Settings::KeyValList Settings::Array::get()
{
    return mParent->getArray(mKey);
}
