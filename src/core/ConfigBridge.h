#ifndef CONFIGBRIDGE_H
#define CONFIGBRIDGE_H

#include <QObject>
#include <QString>
#include <QVariant>

class ConfigBridge : public QObject
{
    Q_OBJECT
public:
    explicit ConfigBridge(QObject *parent = nullptr);

    Q_INVOKABLE QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setValue(const QString& key, const QVariant& value);
};

#endif
