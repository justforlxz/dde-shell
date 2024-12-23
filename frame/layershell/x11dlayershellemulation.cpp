// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "dlayershellwindow.h"
#include "x11dlayershellemulation.h"

#include <QScreen>
#include <QMargins>
#include <QGuiApplication>
#include <QLoggingCategory>

#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformscreen_p.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

DS_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(layershell, "dde.shell.layershell")

LayerShellEmulation::LayerShellEmulation(QWindow* window, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_dlayerShellWindow(DLayerShellWindow::get(m_window))
{
    onLayerChanged();
    connect(m_dlayerShellWindow, &DLayerShellWindow::layerChanged, this, &LayerShellEmulation::onLayerChanged);

    onPositionChanged();
    connect(m_dlayerShellWindow, &DLayerShellWindow::anchorsChanged, this, &LayerShellEmulation::onPositionChanged);
    connect(m_dlayerShellWindow, &DLayerShellWindow::marginsChanged, this, &LayerShellEmulation::onPositionChanged);

    onExclusionZoneChanged();
    connect(m_dlayerShellWindow, &DLayerShellWindow::anchorsChanged, this, &LayerShellEmulation::onExclusionZoneChanged);
    connect(m_dlayerShellWindow, &DLayerShellWindow::exclusionZoneChanged, this, &LayerShellEmulation::onExclusionZoneChanged);

    // qml height or width may update later, need to update anchor postion and exclusion zone
    connect(m_window, &QWindow::widthChanged, this, &LayerShellEmulation::onExclusionZoneChanged);
    connect(m_window, &QWindow::widthChanged, this, &LayerShellEmulation::onPositionChanged);
    // (x,y) wasn't set correctly by xcb_configure_window, TODO using EventFilter to update positions.
    connect(m_window, &QWindow::xChanged, this, &LayerShellEmulation::onPositionChanged);
    connect(m_window, &QWindow::yChanged, this, &LayerShellEmulation::onPositionChanged);

    connect(m_window, &QWindow::heightChanged, this, &LayerShellEmulation::onExclusionZoneChanged);
    connect(m_window, &QWindow::heightChanged, this, &LayerShellEmulation::onPositionChanged);

    auto screen = m_window->screen();
    connect(screen, &QScreen::geometryChanged, this, &LayerShellEmulation::onPositionChanged);
    connect(screen, &QScreen::geometryChanged, this, &LayerShellEmulation::onExclusionZoneChanged);
    connect(qApp, &QGuiApplication::primaryScreenChanged, this, &LayerShellEmulation::onExclusionZoneChanged);
    connect(m_window, &QWindow::screenChanged, this, [this](QScreen *nowScreen){
        for (auto screen : qApp->screens()) {
            screen->disconnect(this);
        }

        connect(nowScreen, &QScreen::geometryChanged, this, &LayerShellEmulation::onPositionChanged);
        connect(nowScreen, &QScreen::geometryChanged, this, &LayerShellEmulation::onExclusionZoneChanged);
        onPositionChanged();
        QMetaObject::invokeMethod(this, &LayerShellEmulation::onExclusionZoneChanged, Qt::QueuedConnection);
    });

    // connect(m_dlayerShellWindow, &DS_NAMESPACE::DLayerShellWindow::keyboardInteractivityChanged, this, &LayerShellEmulation::onKeyboardInteractivityChanged);
}

/**
  * https://specifications.freedesktop.org/wm-spec/wm-spec-1.4.html#STACKINGORDER
  * the following layered stacking order is recommended, from the bottom
  * _NET_WM_TYPE_DESKTOP
  * _NET_WM_STATE_BELOW
  * windows not belonging in any other layer
  * _NET_WM_TYPE_DOCK (unless they have state _NET_WM_TYPE_BELOW) and windows having state _NET_WM_STATE_ABOVE
  * focused windows having state _NET_WM_STATE_FULLSCREEN
  */
void LayerShellEmulation::onLayerChanged()
{
    auto xcbWindow = dynamic_cast<QNativeInterface::Private::QXcbWindow*>(m_window->handle());
    switch (m_dlayerShellWindow->layer()) {
        case DLayerShellWindow::LayerBackground: {
            m_window->setFlags(m_window->flags() & ~Qt::WindowStaysOnBottomHint);
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Desktop);
            break;
        }
        case DLayerShellWindow::LayerButtom: {
            //use Normal type will influenced by exclusionZone
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Normal);
            m_window->setFlags(Qt::WindowStaysOnBottomHint);
            break;
        }
        case DLayerShellWindow::LayerTop: {
            m_window->setFlags(m_window->flags() & ~Qt::WindowStaysOnBottomHint);
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Dock);
            break;
        }
        case DLayerShellWindow::LayerOverlay: {
            // on deepin Notification will be influenced by exclusionZone,
            // while plasma works all right, maybe deepin kwin bug?
            // FIXME: fix above
            m_window->setFlags(m_window->flags() & ~Qt::WindowStaysOnBottomHint);
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Notification);
            break;
        }
    }
}

void LayerShellEmulation::onPositionChanged()
{
    // Work around QPA abstraction so we can use physical pixels, not logical pixels after Qt DPI transformation
    auto platformScreen = m_window->screen()->handle();
    qreal factor = QHighDpiScaling::factor(platformScreen);

    // These geometries are in physical pixels
    auto windowGeometry = m_window->handle()->geometry();
    auto screenGeom = platformScreen->geometry();

    auto anchors = m_dlayerShellWindow->anchors();
    auto x = screenGeom.left() + (screenGeom.width() - windowGeometry.width()) / 2;
    auto y = screenGeom.top() + (screenGeom.height() - windowGeometry.height()) / 2;

    // Set
    if (anchors & DLayerShellWindow::AnchorRight) {
        // https://doc.qt.io/qt-6/qrect.html#right
        x = (screenGeom.right() + 1 - windowGeometry.width() - qRound(m_dlayerShellWindow->rightMargin() * factor));
    }

    if (anchors & DLayerShellWindow::AnchorBottom) {
        // https://doc.qt.io/qt-6/qrect.html#bottom
        y = (screenGeom.bottom() + 1 - windowGeometry.height() - qRound(m_dlayerShellWindow->bottomMargin() * factor));
    }
    if (anchors & DLayerShellWindow::AnchorLeft) {
        x = (screenGeom.left() + qRound(m_dlayerShellWindow->leftMargin() * factor));
    }
    if (anchors & DLayerShellWindow::AnchorTop) {
        y = (screenGeom.top() + qRound(m_dlayerShellWindow->topMargin() * factor));
    }

    QRect rect(x, y, windowGeometry.width(), windowGeometry.height());

    const bool horizontallyConstrained = anchors.testFlags({DLayerShellWindow::AnchorLeft, DLayerShellWindow::AnchorRight});
    const bool verticallyConstrained = anchors.testFlags({DLayerShellWindow::AnchorTop, DLayerShellWindow::AnchorBottom});

    if (horizontallyConstrained) {
        rect.setX(screenGeom.left() + qRound(m_dlayerShellWindow->leftMargin() * factor));
        rect.setWidth(screenGeom.width() - qRound((m_dlayerShellWindow->leftMargin() + m_dlayerShellWindow->rightMargin()) * factor));
    }
    if (verticallyConstrained) {
        rect.setY(screenGeom.top() + qRound(m_dlayerShellWindow->topMargin() * factor));
        rect.setHeight(screenGeom.height() - qRound((m_dlayerShellWindow->topMargin() + m_dlayerShellWindow->bottomMargin()) * factor));
    }

    // Configure window geometry via XCB interface
    auto x11App = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    auto xcbConnection = x11App->connection();
    auto xcbWindow = m_window->winId();
    uint16_t mask = 0;
    struct {
        int32_t x;
        int32_t y;
        uint32_t width;
        uint32_t height;
    } targetWindowRect;

    targetWindowRect.x = rect.x();
    targetWindowRect.y = rect.y();
    targetWindowRect.width = rect.width();
    targetWindowRect.height = rect.height();
    mask |= XCB_CONFIG_WINDOW_X;
    mask |= XCB_CONFIG_WINDOW_Y;
    mask |= XCB_CONFIG_WINDOW_WIDTH;
    mask |= XCB_CONFIG_WINDOW_HEIGHT;

    xcb_configure_window(xcbConnection, xcbWindow, mask, &targetWindowRect);
    xcb_flush(xcbConnection);
}

/**
  * https://specifications.freedesktop.org/wm-spec/wm-spec-1.4.html#idm45649101327728
  */
void LayerShellEmulation::onExclusionZoneChanged()
{
    auto scaleFactor = qGuiApp->devicePixelRatio();
    auto *x11Application = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    xcb_ewmh_connection_t ewmh_connection;
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(x11Application->connection(), &ewmh_connection);
    xcb_ewmh_init_atoms_replies(&ewmh_connection, cookie, NULL);
    xcb_ewmh_wm_strut_partial_t strut_partial;
    memset(&strut_partial, 0, sizeof(xcb_ewmh_wm_strut_partial_t));
    auto anchors = m_dlayerShellWindow->anchors();
    QRect rect = m_window->screen()->geometry();
    if ((anchors == DLayerShellWindow::AnchorLeft) || (anchors ^ DLayerShellWindow::AnchorLeft) == (DLayerShellWindow::AnchorTop | DLayerShellWindow::AnchorBottom)) {
        strut_partial.left = rect.x() + (m_dlayerShellWindow->exclusionZone()) * scaleFactor;
        strut_partial.left_start_y = rect.y();
        strut_partial.left_end_y = rect.y() + m_window->height();
    } else if ((anchors == DLayerShellWindow::AnchorRight) || (anchors ^ DLayerShellWindow::AnchorRight) == (DLayerShellWindow::AnchorTop | DLayerShellWindow::AnchorBottom)) {
        int boundary = 0;
        for (auto screen : qApp->screens()) {
            if (boundary < screen->geometry().right())
                boundary = screen->geometry().right();
        }
        strut_partial.right = boundary - rect.right() + (m_dlayerShellWindow->exclusionZone()) * scaleFactor;
        strut_partial.right_start_y = rect.y();
        strut_partial.right_end_y = rect.y() + m_window->height();
    } else if ((anchors == DLayerShellWindow::AnchorTop) || (anchors ^ DLayerShellWindow::AnchorTop) == (DLayerShellWindow::AnchorLeft | DLayerShellWindow::AnchorRight)) {
        strut_partial.top = rect.y() + (m_dlayerShellWindow->exclusionZone()) * scaleFactor;
        strut_partial.top_start_x = rect.x();
        strut_partial.top_end_x = rect.x() + m_window->width();
    } else if ((anchors == DLayerShellWindow::AnchorBottom) || (anchors ^ DLayerShellWindow::AnchorBottom) == (DLayerShellWindow::AnchorLeft | DLayerShellWindow::AnchorRight)) {
        // Note: In the X environment, 
        // the upper screen's exclusive zone spans across the entire lower screen when there are multiple screens, 
        // but there is no issue.
        int boundary = 0;
        for (auto screen : qApp->screens()) {
            if (boundary < screen->geometry().bottom())
                boundary = screen->geometry().bottom();
        }
        strut_partial.bottom =  + boundary - rect.bottom() + (m_dlayerShellWindow->exclusionZone()) * scaleFactor;
        strut_partial.bottom_start_x = rect.x();
        strut_partial.bottom_end_x = rect.x() + m_window->width();
    }

    qCDebug(layershell) << "update exclusion zone, winId:" << m_window->winId()
                        << ", (left, right, top, bottom)"
                        << strut_partial.left << strut_partial.right << strut_partial.top << strut_partial.bottom;
    xcb_ewmh_set_wm_strut_partial(&ewmh_connection, m_window->winId(), strut_partial);
}

// void X11Emulation::onKeyboardInteractivityChanged()
// {
//     // kwin no implentation on wayland
// }
DS_END_NAMESPACE
