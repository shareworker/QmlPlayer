#include "ConfigManager.h"
#include <QCoreApplication>

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager()
    : settings_(QCoreApplication::organizationName(), QCoreApplication::applicationName())
{
}

QVariant ConfigManager::value(const QString& key, const QVariant& defaultValue) const
{
    return settings_.value(key, defaultValue);
}

void ConfigManager::setValue(const QString& key, const QVariant& value)
{
    settings_.setValue(key, value);
}
