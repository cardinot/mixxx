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
    void slotDownloadFinished();
    void slotSearchFinished();

  private:
    struct SearchResult {
      QString artist;
      QString album;
      QPixmap cover;
    };

    TrackPointer m_track;
    QNetworkAccessManager* m_pNetworkManager;
    QNetworkReply* m_pLastDownloadReply;
    QNetworkReply* m_pLastSearchReply;
    QString m_sLastRequestedAlbum;
    QString m_sLastRequestedArtist;
    QMap<QString, SearchResult> m_searchresults;

    void abortSearch();
    void setStatusOfSearchBtn(bool isSearching);
    void parseAlbum(QXmlStreamReader& xml);
    void downloadNextCover();
    void showResults();
};

#endif // DLGCOVERARTFETCHER_H
