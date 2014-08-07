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
    bool changeCoverArt(int trackId, const QString& newCoverLocation);
    QPixmap requestPixmap(int trackId,
                          const QString& coverLocation = QString(),
                          const QString& md5Hash = QString(),
                          const bool tryLoadAndSearch = true,
                          const bool croppedPixmap = false,
                          const bool issueRepaint = false);
    void setCoverArtDAO(CoverArtDAO* coverdao);
    void setTrackDAO(TrackDAO* trackdao);
    QString getDefaultCoverLocation() { return m_sDefaultCoverLocation; }
    QPixmap getDefaultCoverArt() { return m_defaultCover; }

  public slots:
    void imageFound();
    void imageLoaded();

  private slots:
    void updateDB();

  signals:
    void pixmapFound(int trackId, QPixmap pixmap);
    void requestRepaint();

  protected:
    CoverArtCache();
    virtual ~CoverArtCache();
    friend class Singleton<CoverArtCache>;

    struct FutureResult {
        int trackId;
        QString coverLocation;
        QString md5Hash;
        QImage img;
        bool croppedImg;
        bool issueRepaint;
        bool newImgFound;
    };

    FutureResult searchImage(CoverArtDAO::CoverArtInfo coverInfo,
                             const bool croppedPixmap = false,
                             const bool emitSignals = true);
    FutureResult loadImage(int trackId,
                           const QString& coverLocation,
                           const QString& md5Hash,
                           const bool croppedPixmap = false,
                           const bool emitSignals = true);

  private:
    static CoverArtCache* m_instance;
    CoverArtDAO* m_pCoverArtDAO;
    TrackDAO* m_pTrackDAO;
    const QString m_sDefaultCoverLocation;
    const QPixmap m_defaultCover;
    QTimer* m_timer;
    QSet<int> m_runningIds;
    QHash<int, QPair<QString, QString> > m_queueOfUpdates;

    QString calculateMD5(QImage img);
    QImage cropImage(QImage img);
    QImage rescaleBigImage(QImage img);
    QImage extractEmbeddedCover(QString trackLocation);
    QString searchInTrackDirectory(QString directory,
                                   QString trackBaseName,
                                   QString album);
};

#endif // COVERARTCACHE_H
