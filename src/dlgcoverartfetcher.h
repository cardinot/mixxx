#ifndef DLGCOVERARTFETCHER_H
#define DLGCOVERARTFETCHER_H

#include <QDialog>
#include <QNetworkReply>
#include <QXmlStreamReader>

#include "trackinfoobject.h"
#include "ui_dlgcoverartfetcher.h"

const QString APIKEY_LASTFM = "0082384c4beaad1f558827183cdc001f";

class DlgCoverArtFetcher : public QDialog, public Ui::DlgCoverArtFetcher {
    Q_OBJECT
  public:
    DlgCoverArtFetcher(QWidget *parent);
    virtual ~DlgCoverArtFetcher();

    void init(const TrackPointer track);

  private slots:
    void slotCancel();
    void slotSearch();
    void slotSearchFinished();

  private:
    struct SearchResult {
      QString artist;
      QString album;
      QUrl cover_url;
    };

    TrackPointer m_track;
    QNetworkReply* m_pLastReply;
    QString m_sLastRequestedAlbum;
    QString m_sLastRequestedArtist;
    QList<SearchResult> m_searchresults;

    SearchResult parseAlbum(QXmlStreamReader& xml);
};

#endif // DLGCOVERARTFETCHER_H
