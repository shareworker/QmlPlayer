#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QSettings>
#include <QString>
#include <QVariant>

class ConfigManager
{
public:
    static ConfigManager& instance();

    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);

private:
    ConfigManager();

    QSettings settings_;
};

#endif
