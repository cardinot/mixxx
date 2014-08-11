#include <QStringBuilder>
#include <QtDebug>
#include <QToolButton>

#include "dlgcoverartfetcher.h"
#include "library/coverartcache.h"

DlgCoverArtFetcher::DlgCoverArtFetcher(QWidget *parent)
        : QDialog(parent),
          m_pTrack(NULL),
          m_pNetworkManager(new QNetworkAccessManager(this)),
          m_pLastDownloadReply(NULL),
          m_pLastSearchReply(NULL),
          m_kCellSize(110, 140),
          m_kCoverSize(100, 100) {
    setupUi(this);

    connect(btnSearch, SIGNAL(clicked()),
            this, SLOT(slotSearch()));
    connect(btnClose, SIGNAL(clicked()),
            this, SLOT(slotClose()));
    connect(btnApply, SIGNAL(clicked()),
            this, SLOT(slotApply()));
    connect(btnPrev, SIGNAL(clicked()),
            this, SIGNAL(previous()));
    connect(btnNext, SIGNAL(clicked()),
            this, SIGNAL(next()));

    connect(txtAlbum, SIGNAL(editingFinished()),
            this, SLOT(slotEditingFinished()));
    connect(txtArtist, SIGNAL(editingFinished()),
            this, SLOT(slotEditingFinished()));

    btnSearch->setCheckable(true);

    coverView->setIconSize(m_kCoverSize);
    coverView->setGridSize(m_kCellSize);

    coverView->setFont(QFont("Sans Serif", 7));
    coverView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    coverView->setFlow(QListView::LeftToRight);
    coverView->setWrapping(true);
    coverView->setResizeMode(QListView::Adjust);
    coverView->setViewMode(QListView::IconMode);
    coverView->setUniformItemSizes(true);
    coverView->setWordWrap(true);
}

DlgCoverArtFetcher::~DlgCoverArtFetcher() {
    qDebug() << "~DlgCoverArtFetcher()";
}

void DlgCoverArtFetcher::init(const TrackPointer track) {
    abortSearch();
    m_downloadQueue.clear();
    coverView->clear();
    m_pTrack = track;

    if (track->getAlbum().isEmpty() && track->getArtist().isEmpty()) {
        txtAlbum->setText(track->getTitle());
    } else {
        txtAlbum->setText(track->getAlbum());
        txtArtist->setText(track->getArtist());
    }

    if (!txtAlbum->text().isEmpty() || !txtArtist->text().isEmpty()) {
        btnSearch->setChecked(true);
        slotSearch();
    }
}

void DlgCoverArtFetcher::abortSearch() {
    if (m_pLastSearchReply != NULL) {
        m_pLastSearchReply->abort();
        m_pLastSearchReply->deleteLater();
        m_pLastSearchReply = NULL;
    }
    if (m_pLastDownloadReply != NULL) {
        m_pLastDownloadReply->abort();
        m_pLastDownloadReply->deleteLater();
        m_pLastDownloadReply = NULL;
    }
    setStatus(READY);
}

void DlgCoverArtFetcher::setStatus(Status status, bool onlyStatusField) {
    QString statusText;
    QString btnText = tr("Search");
    bool enableFields = true;
    switch (status) {
        case READY:
            statusText = tr("ready!");
            break;
        case NOTFOUND:
            statusText = tr("No images were found!");
            break;
        case SEARCHING:
            statusText = tr("searching...");
            btnText = tr("Abort");
            enableFields = false;
            break;
    }
    txtStatus->setText(statusText);
    if (!onlyStatusField)
    {
        btnSearch->setText(btnText);
        btnSearch->setChecked(!enableFields);
        txtArtist->setEnabled(enableFields);
        txtAlbum->setEnabled(enableFields);
    }
}

void DlgCoverArtFetcher::slotEditingFinished() {
    setStatus(READY, true);
}

void DlgCoverArtFetcher::slotClose() {
    abortSearch();
    close();
}

void DlgCoverArtFetcher::slotSearch() {
    if (!btnSearch->isChecked()) {
        abortSearch();
        return;
    }

    m_downloadQueue.clear();
    coverView->clear();
    setStatus(SEARCHING);

    // Last.fm
    QUrl url;
    url.setScheme("http");
    url.setHost("ws.audioscrobbler.com");
    url.setPath("/2.0/");
    url.addQueryItem("method", "album.search");
    url.addQueryItem("album", txtAlbum->text() % " " % txtArtist->text());
    url.addQueryItem("api_key", APIKEY_LASTFM);
    m_pLastSearchReply = m_pNetworkManager->get(QNetworkRequest(url));
    connect(m_pLastSearchReply, SIGNAL(finished()),
            this, SLOT(slotSearchFinished()), Qt::UniqueConnection);
}

void DlgCoverArtFetcher::slotSearchFinished() {
    if (m_pLastSearchReply == NULL) {
        setStatus(READY);
        return;
    }

    if (m_pLastSearchReply->error() != QNetworkReply::NoError) {
        abortSearch();
        return;
    }

    SearchResult res;
    QXmlStreamReader xml(m_pLastSearchReply->readAll());
    while(!xml.atEnd() && !xml.hasError())
    {
        xml.readNext();
        if(xml.name() == "album") {
            parseAlbum(xml, res.album, res.artist, res.url);
            if (res.url.isValid()) {
                m_downloadQueue.append(res);
            }
        }
    }

    if (m_downloadQueue.isEmpty()) {
        setStatus(NOTFOUND);
    } else {
        downloadNextCover();
    }

    m_pLastSearchReply->deleteLater();
    m_pLastSearchReply = NULL;
}

void DlgCoverArtFetcher::parseAlbum(QXmlStreamReader& xml, QString& album,
                                    QString& artist, QUrl& url) const {
    if (xml.name() != "album") {
        return;
    }

    while(!xml.atEnd() && !xml.hasError())
    {
        xml.readNext();
        const QStringRef &name = xml.name();

        if (name == "album") {
            break;
        }

        if (name == "name") {
            album = xml.readElementText();
        } else if (name == "artist") {
            artist = xml.readElementText();
        } else if (name == "image") {
            if (xml.attributes().value("size").toString() == "extralarge") {
                url = xml.readElementText();
            }
        }
    }
}

void DlgCoverArtFetcher::downloadNextCover() {
    if (m_downloadQueue.isEmpty()) {
        setStatus(READY);
        return;
    }

    m_pLastDownloadReply = m_pNetworkManager->get(
                QNetworkRequest(m_downloadQueue.first().url));

    connect(m_pLastDownloadReply, SIGNAL(finished()),
            this, SLOT(slotDownloadFinished()), Qt::UniqueConnection);
}

void DlgCoverArtFetcher::slotDownloadFinished() {
    if (m_downloadQueue.isEmpty() || m_pLastDownloadReply == NULL) {
        setStatus(READY);
        return;
    }

    if (m_pLastDownloadReply->error() == QNetworkReply::NoError) {
        QPixmap pixmap;
        if (pixmap.loadFromData(m_pLastDownloadReply->readAll())) {
            QString album = m_downloadQueue.first().album;
            QString artist = m_downloadQueue.first().artist;
            QString title = album.left(16) % "\n" % artist.left(16);
            QListWidgetItem* cell = new QListWidgetItem(pixmap, title, coverView);
            cell->setToolTip(album);
        }
    }

    m_pLastDownloadReply->deleteLater();
    m_pLastDownloadReply = NULL;
    m_downloadQueue.removeFirst();
    downloadNextCover();
}

void DlgCoverArtFetcher::slotApply() {
    if (m_pTrack == NULL) {
        return;
    }

    QString trackPath = m_pTrack->getDirectory();
    QString newCoverLocation;
    QStringList filepaths;
    filepaths << trackPath % "/cover.jpg"
              << trackPath % "/album.jpg"
              << trackPath % "/mixxx-cover.jpg";
    foreach (QString filepath, filepaths) {
        if (!QFile::exists(filepath)) {
            newCoverLocation = filepath;
            break;
        }
    }
    if (newCoverLocation.isEmpty()) {
        // overwrites "mixxx-cover"
        newCoverLocation = filepaths.last();
        QFile::remove(newCoverLocation);
    }

    bool res = false;
    if (coverView->currentItem()->icon().pixmap(1000).save(newCoverLocation)) {
        res = CoverArtCache::instance()->changeCoverArt(m_pTrack->getId(),
                                                        newCoverLocation);
    }

    if (!res) {
        QMessageBox::warning(this, tr("Cover Art"),
                             tr("Could not change the cover art!"));
        return;
    }

    slotClose();
}
