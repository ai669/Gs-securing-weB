#include "notificator.h"

#include <QMetaType>
#include <QVariant>
#include <QIcon>
#include <QApplication>
#include <QStyle>
#include <QByteArray>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QImageWriter>

#ifdef USE_DBUS
#include <QtDBus>
#include <stdint.h>
#endif

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#include "macnotificationhandler.h"
#endif

// https://wiki.ubuntu.com/NotificationDevelopmentGuidelines recommends at least 128
const int FREEDESKTOP_NOTIFICATION_ICON_SIZE = 128;

Notificator::Notificator(const QString &programName, QSystemTrayIcon *trayicon, QWidget *parent):
    QObject(parent),
    parent(parent),
    programName(programName),
    mode(None),
    trayIcon(trayicon)
#ifdef USE_DBUS
    ,interface(0)
#endif
{
    if(trayicon && trayicon->supportsMessages())
    {
        mode = QSystemTray;
    }
#ifdef USE_DBUS
    interface = new QDBusInterface("org.freedesktop.Notifications",
          "/org/freedesktop/Notifications", "org.freedesktop.Notifications");
    if(interface->isValid())
    {
        mode = Freedesktop;
    }
#endif
#ifdef Q_OS_MAC
    // check if users OS has support for NSUserNotification
    if( MacNotificationHandler::instance()->hasUserNotificationCenterSupport()) {
        mode = UserNotificationCenter;
    }
#endif
}

Notificator::~Notificator()
{
#ifdef USE_DBUS
    delete interface;
#endif
}

#ifdef USE_DBUS

// Loosely based on http://www.qtcentre.org/archive/index.php/t-25879.html
class FreedesktopImage
{
public:
    FreedesktopImage() {}
    FreedesktopImage(const QImage &img);

    static int metaType();

    // Image to variant that can be marshalled over DBus
    static QVariant toVariant(const QImage &img);

private:
    int width, height, stride;
    bool hasAlpha;
    int channels;
    int bitsPerSample;
    QByteArray image;

    friend QDBusArgument &operator<<(QDBusArgument &a, const FreedesktopImag