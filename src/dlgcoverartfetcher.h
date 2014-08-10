#ifndef DLGCOVERARTFETCHER_H
#define DLGCOVERARTFETCHER_H

#include <QButtonGroup>
#include <QDialog>
#include <QNetworkReply>
#include <QStandardItemModel>
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

  signals:
    void next();
    void previous();

  private slots:
    void slotApply(QAbstractButton* cell);
    void slotClose();
    void slotSearch();
    void slotEditingFinished();
    void slotDownloadFinished();
    void slotSearchFinished();

  private:
    enum Status {
        READY,
        NOTFOUND,
        SEARCHING
    };

    struct SearchResult {
      QString artist;
      QString album;
      QUrl url;
    };

    TrackPointer m_pTrack;
    QNetworkAccessManager* m_pNetworkManager;
    QNetworkReply* m_pLastDownloadReply;
    QNetworkReply* m_pLastSearchReply;

    QList<SearchResult> m_downloadQueue;

    const QSize m_kCellSize;
    const QSize m_kCoverSize;

    void initCoverView();
    void setStatus(Status status);
    void abortSearch();
    void downloadNextCover();
    void showResult(SearchResult res, QPixmap pixmap);
    void getNextPosition(bool& newRow, int& row, int& column);
    void parseAlbum(QXmlStreamReader& xml, QString& album,
                    QString& artist, QUrl& url) const;
};

#endif // DLGCOVERARTFETCHER_H
