#include "ConfigBridge.h"
#include "ConfigManager.h"

ConfigBridge::ConfigBridge(QObject *parent)
    : QObject(parent)
{
}

QVariant ConfigBridge::getValue(const QString& key, const QVariant& defaultValue) const
{
    return ConfigManager::instance().value(key, defaultValue);
}

void ConfigBridge::setValue(const QString& key, const QVariant& value)
{
    ConfigManager::instance().setValue(key, value);
}
