#ifndef COVERARTCACHE_H
#define COVERARTCACHE_H

#include <QImage>
#include <QFutureWatcher>
#include <QObject>
#include <QPixmapCache>

#include "library/dao/coverartdao.h"
#include "library/dao/trackdao.h"
#include "util/singleton.h"

class CoverArtCache : public QObject, public Singleton<CoverArtCache>
{
    Q_OBJECT
  public:
    void requestPixmap(int trackId,
                       QPixmap& pixmap,
                       const QString& coverLocation = QString());
    void setCoverArtDAO(CoverArtDAO* coverdao);
    void setTrackDAO(TrackDAO* trackdao);
    QString getDefaultCoverLocation(int trackId);

  public slots:
    void imageFound();
    void imageLoaded();

  signals:
    void pixmapFound(int trackId);

  protected:
    CoverArtCache();
    virtual ~CoverArtCache();
    friend class Singleton<CoverArtCache>;

    struct FutureResult {
        int trackId;
        QString coverLocation;
        QImage img;
    };

    FutureResult loadImage(int trackId, const QString& coverLocation);
    FutureResult searchImage(CoverArtDAO::CoverArtInfo coverInfo);

  private:
    static CoverArtCache* m_instance;
    CoverArtDAO* m_pCoverArtDAO;
    TrackDAO* m_pTrackDAO;
    QSet<int> m_runningIds;
    QPixmap* m_pixmap;

    QImage rescaleBigImage(QImage img);
    QImage searchEmbeddedCover(QString trackLocation);
    QString searchInTrackDirectory(QString directory,
                                   QString trackBaseName,
                                   QString album);
};

#endif // COVERARTCACHE_H
