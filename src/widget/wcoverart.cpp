#include <QApplication>
#include <QBitmap>
#include <QLabel>
#include <QPainter>

#include "library/coverartcache.h"
#include "wcoverart.h"
#include "wskincolor.h"

WCoverArt::WCoverArt(QWidget* parent,
                     ConfigObject<ConfigValue>* pConfig)
        : QWidget(parent),
          WBaseWidget(this),
          m_pConfig(pConfig),
          m_bCoverIsHovered(false),
          m_bCoverIsVisible(false),
          m_bDefaultCover(true) {
    // initialize cover art images
    m_defaultCover = QPixmap(":/images/library/vinyl-record.png");
    m_currentCover = m_defaultCover;
    m_currentScaledCover = m_defaultCover;

    // load icon to hide cover
    m_iconHide = QPixmap(":/images/library/ic_library_cover_hide.png");
    m_iconHide = m_iconHide.scaled(20,
                                   20,
                                   Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation);

    // load icon to show cover
    m_iconShow = QPixmap(":/images/library/ic_library_cover_show.png");
    m_iconShow = m_iconShow.scaled(17,
                                   17,
                                   Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation);

    // load zoom cursor
    QPixmap zoomImg(":/images/library/ic_library_zoom_in.png");
    zoomImg = zoomImg.scaled(24, 24);
    m_zoomCursor = QCursor(zoomImg);

    connect(CoverArtCache::instance(), SIGNAL(pixmapFound(int)),
            this, SLOT(slotPixmapFound(int)));
}

WCoverArt::~WCoverArt() {
}

void WCoverArt::setup(QDomNode node, const SkinContext& context) {
    Q_UNUSED(node);
    setMouseTracking(TRUE);

    // Background color
    QColor bgc(255,255,255);
    if (context.hasNode(node, "BgColor")) {
        bgc.setNamedColor(context.selectString(node, "BgColor"));
        setAutoFillBackground(true);
    }
    QPalette pal = palette();
    pal.setBrush(backgroundRole(), WSkinColor::getCorrectColor(bgc));

    // Foreground color
    QColor m_fgc(0,0,0);
    if (context.hasNode(node, "FgColor")) {
        m_fgc.setNamedColor(context.selectString(node, "FgColor"));
    }
    bgc = WSkinColor::getCorrectColor(bgc);
    m_fgc = QColor(255 - bgc.red(), 255 - bgc.green(), 255 - bgc.blue());
    pal.setBrush(foregroundRole(), m_fgc);
    setPalette(pal);
}

void WCoverArt::setToDefault() {
    m_sCoverTitle = "Cover Art";
    m_currentCover = m_defaultCover;
    m_currentScaledCover = m_defaultCover;
    m_bDefaultCover = true;
    update();
}

void WCoverArt::slotHideCoverArt() {
    m_bCoverIsVisible = false;
    setMinimumSize(0, 20);
    setToDefault();
}

void WCoverArt::slotPixmapFound(int trackId) {
    if (m_lastRequestedTrackId == trackId) {
        //m_sCoverTitle = location.mid(location.lastIndexOf("/") + 1);
        m_currentCover = m_lastRequestedPixmap;
        m_currentScaledCover = m_currentCover;//scaledCoverArt(m_currentCover);
        m_bDefaultCover = false;
        update();
    }
}

void WCoverArt::slotLoadCoverArt(const QString& coverLocation, int trackId) {
    if (!m_bCoverIsVisible) {
        return;
    }

    setToDefault();

    m_lastRequestedTrackId = trackId;
    m_lastRequestedPixmap = QPixmap();
    CoverArtCache::instance()->requestPixmap(trackId,
                                             m_lastRequestedPixmap,
                                             coverLocation);
}

QPixmap WCoverArt::scaledCoverArt(QPixmap normal) {
    int height = parentWidget()->height() / 3;
    return normal.scaled(QSize(height - 10, height - 10),
                         Qt::KeepAspectRatioByExpanding,
                         Qt::SmoothTransformation);
}

void WCoverArt::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.drawLine(0, 0, width(), 0);

    if (m_bCoverIsVisible) {
        painter.drawPixmap(width() / 2 - height() / 2 + 4, 6,
                           m_currentScaledCover);
    } else {
        painter.drawPixmap(1, 2 ,m_iconShow);
        painter.drawText(25, 15, tr("Show Cover Art"));
    }

    if (m_bCoverIsVisible && m_bCoverIsHovered) {
        painter.drawPixmap(width() - 21, 6, m_iconHide);
    }
}

void WCoverArt::resizeEvent(QResizeEvent*) {
    if (m_bCoverIsVisible) {
        setMinimumSize(0, parentWidget()->height() / 3);
     } else {
        slotHideCoverArt();
    }
}

void WCoverArt::mousePressEvent(QMouseEvent* event) {
    QPoint lastPoint;
    lastPoint = event->pos();
    if (m_bCoverIsVisible) {
        if(lastPoint.x() > width() - (height() / 5)
                && lastPoint.y() < (height() / 5) + 5) {
            m_bCoverIsVisible = false;
            resize(sizeHint());
        } else {
            // When the current cover is not a default one,
            // it'll show the cover in a full size
            // (in a new window - by left click)
            if (!m_bDefaultCover) {
                QLabel *lb = new QLabel(this, Qt::Popup |
                                              Qt::Tool |
                                              Qt::CustomizeWindowHint |
                                              Qt::WindowCloseButtonHint);
                lb->setWindowModality(Qt::ApplicationModal);
                lb->setWindowTitle(m_sCoverTitle);

                QPixmap px = m_currentCover;
                QSize sz = QApplication::activeWindow()->size();

                if (px.height() > sz.height() / 1.2) {
                    px = px.scaledToHeight(sz.height() / 1.2,
                                           Qt::SmoothTransformation);
                }

                lb->setPixmap(px);
                lb->setGeometry(sz.width() / 2 - px.width() / 2,
                                sz.height() / 2 - px.height() / 2.2,
                                px.width(),
                                px.height());
                lb->show();
            }
        }
    } else {
        m_bCoverIsVisible = true;
        setCursor(Qt::ArrowCursor);
        resize(sizeHint());
    }
}

void WCoverArt::mouseMoveEvent(QMouseEvent* event) {
    QPoint lastPoint;
    lastPoint = event->pos();
    if (event->HoverEnter) {
        m_bCoverIsHovered  = true;
        if (m_bCoverIsVisible) {
            if (lastPoint.x() > width() - (height() / 5)
                    && lastPoint.y() < (height() / 5) + 5) {
                setCursor(Qt::ArrowCursor);
            } else {
                if (m_bDefaultCover) {
                    setCursor(Qt::ArrowCursor);
                } else {
                    setCursor(m_zoomCursor);
                }
            }
        } else {
            setCursor(Qt::PointingHandCursor);
        }
    }
    update();
}

void WCoverArt::leaveEvent(QEvent*) {
    m_bCoverIsHovered = false;
    update();
}
