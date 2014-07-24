
#ifndef DLGTRACKINFO_H
#define DLGTRACKINFO_H

#include <QDialog>
#include <QMutex>
#include <QHash>
#include <QList>

#include "ui_dlgtrackinfo.h"
#include "dlgcoverartfetcher.h"
#include "dlgtagfetcher.h"
#include "trackinfoobject.h"
#include "util/types.h"

const int kFilterLength = 5;

class Cue;

class DlgTrackInfo : public QDialog, public Ui::DlgTrackInfo {
    Q_OBJECT
  public:
    DlgTrackInfo(QWidget* parent,
                 DlgTagFetcher& DlgTagFetcher,
                 DlgCoverArtFetcher& DlgCoverArtFetcher);
    virtual ~DlgTrackInfo();

  public slots:
    // Not thread safe. Only invoke via AutoConnection or QueuedConnection, not
    // directly!
    void loadTrack(TrackPointer pTrack);
    void slotLoadCoverArt(const QString& coverLocation,
                          const QString& md5Hash,
                          int trackId);

  signals:
    void next();
    void previous();

  private slots:
    void slotNext();
    void slotPrev();
    void OK();
    void apply();
    void cancel();
    void trackUpdated();
    void fetchCover();
    void fetchTag();

    void cueActivate();
    void cueDelete();

    void slotBpmDouble();
    void slotBpmHalve();
    void slotBpmTwoThirds();
    void slotBpmThreeFourth();
    void slotBpmTap();

    void reloadTrackMetadata();
    void slotOpenInFileBrowser();

    void slotPixmapFound(int trackId, QPixmap pixmap);
    void slotChangeCoverArt();
    void slotUnsetCoverArt();
    void slotReloadCover();

  private:
    void populateFields(TrackPointer pTrack);
    void populateCues(TrackPointer pTrack);
    void saveTrack();
    void unloadTrack(bool save);
    void clear();
    void init();
    QPixmap scaledCoverArt(QPixmap original);

    QHash<int, Cue*> m_cueMap;
    TrackPointer m_pLoadedTrack;
    QString m_sLoadedCoverLocation;

    CSAMPLE m_bpmTapFilter[kFilterLength];
    QTime m_bpmTapTimer;

    QMutex m_mutex;
    DlgCoverArtFetcher& m_DlgCoverArtFetcher;
    DlgTagFetcher& m_DlgTagFetcher;

    enum reloadCoverCases {
        LOAD,
        CHANGE,
        UNSET
    };
};

#endif /* DLGTRACKINFO_H */
