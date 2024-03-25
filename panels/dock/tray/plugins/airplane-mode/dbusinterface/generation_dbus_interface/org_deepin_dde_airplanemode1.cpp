/*
 * This file was generated by qdbusxml2cpp-fix version 0.8
 * Command line was: qdbusxml2cpp-fix -c org_deepin_dde_AirplaneMode1 -p /home/uos/private/github_projects/dde-shell/panels/dock/tray/plugins/airplane-mode/dbusinterface/generation_dbus_interface/org_deepin_dde_airplanemode1 /home/uos/private/github_projects/dde-shell/panels/dock/tray/plugins/airplane-mode/dbusinterface/xml/org.deepin.dde.AirplaneMode1.xml
 *
 * qdbusxml2cpp-fix is Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#include "/home/uos/private/github_projects/dde-shell/panels/dock/tray/plugins/airplane-mode/dbusinterface/generation_dbus_interface/org_deepin_dde_airplanemode1.h"

DCORE_USE_NAMESPACE
/*
 * Implementation of interface class __org_deepin_dde_AirplaneMode1
 */

class __org_deepin_dde_AirplaneMode1Private
{
public:
   __org_deepin_dde_AirplaneMode1Private() = default;

    // begin member variables
    bool BluetoothEnabled;
    bool Enabled;
    bool WifiEnabled;

public:
    QMap<QString, QDBusPendingCallWatcher *> m_processingCalls;
    QMap<QString, QList<QVariant>> m_waittingCalls;
};

__org_deepin_dde_AirplaneMode1::__org_deepin_dde_AirplaneMode1(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : DDBusExtendedAbstractInterface(service, path, staticInterfaceName(), connection, parent)
    , d_ptr(new __org_deepin_dde_AirplaneMode1Private)
{
    connect(this, &__org_deepin_dde_AirplaneMode1::propertyChanged, this, &__org_deepin_dde_AirplaneMode1::onPropertyChanged);

}

__org_deepin_dde_AirplaneMode1::~__org_deepin_dde_AirplaneMode1()
{
    qDeleteAll(d_ptr->m_processingCalls.values());
    delete d_ptr;
}

void __org_deepin_dde_AirplaneMode1::onPropertyChanged(const QString &propName, const QVariant &value)
{
    if (propName == QStringLiteral("BluetoothEnabled"))
    {
        const bool &BluetoothEnabled = qvariant_cast<bool>(value);
        if (d_ptr->BluetoothEnabled != BluetoothEnabled)
        {
            d_ptr->BluetoothEnabled = BluetoothEnabled;
            Q_EMIT BluetoothEnabledChanged(d_ptr->BluetoothEnabled);
        }
        return;
    }

    if (propName == QStringLiteral("Enabled"))
    {
        const bool &Enabled = qvariant_cast<bool>(value);
        if (d_ptr->Enabled != Enabled)
        {
            d_ptr->Enabled = Enabled;
            Q_EMIT EnabledChanged(d_ptr->Enabled);
        }
        return;
    }

    if (propName == QStringLiteral("WifiEnabled"))
    {
        const bool &WifiEnabled = qvariant_cast<bool>(value);
        if (d_ptr->WifiEnabled != WifiEnabled)
        {
            d_ptr->WifiEnabled = WifiEnabled;
            Q_EMIT WifiEnabledChanged(d_ptr->WifiEnabled);
        }
        return;
    }

    qWarning() << "property not handle: " << propName;
    return;
}

bool __org_deepin_dde_AirplaneMode1::bluetoothEnabled()
{
    return qvariant_cast<bool>(internalPropGet("BluetoothEnabled", &d_ptr->BluetoothEnabled));
}

bool __org_deepin_dde_AirplaneMode1::enabled()
{
    return qvariant_cast<bool>(internalPropGet("Enabled", &d_ptr->Enabled));
}

bool __org_deepin_dde_AirplaneMode1::wifiEnabled()
{
    return qvariant_cast<bool>(internalPropGet("WifiEnabled", &d_ptr->WifiEnabled));
}

void __org_deepin_dde_AirplaneMode1::CallQueued(const QString &callName, const QList<QVariant> &args)
{
    if (d_ptr->m_waittingCalls.contains(callName))
    {
        d_ptr->m_waittingCalls[callName] = args;
        return;
    }
    if (d_ptr->m_processingCalls.contains(callName))
    {
        d_ptr->m_waittingCalls.insert(callName, args);
    } else {
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(asyncCallWithArgumentList(callName, args));
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &__org_deepin_dde_AirplaneMode1::onPendingCallFinished);
        d_ptr->m_processingCalls.insert(callName, watcher);
    }
}

void __org_deepin_dde_AirplaneMode1::onPendingCallFinished(QDBusPendingCallWatcher *w)
{
    w->deleteLater();
    const auto callName = d_ptr->m_processingCalls.key(w);
    Q_ASSERT(!callName.isEmpty());
    if (callName.isEmpty())
        return;
    d_ptr->m_processingCalls.remove(callName);
    if (!d_ptr->m_waittingCalls.contains(callName))
        return;
    const auto args = d_ptr->m_waittingCalls.take(callName);
    CallQueued(callName, args);
}