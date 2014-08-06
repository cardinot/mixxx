#include <QStandardItemModel>
#include <QStringBuilder>
#include <QtDebug>
#include <QToolButton>

#include "dlgcoverartfetcher.h"

DlgCoverArtFetcher::DlgCoverArtFetcher(QWidget *parent)
        : QDialog(parent),
          m_track(NULL),
          m_pNetworkManager(new QNetworkAccessManager(this)),
          m_pLastDownloadReply(NULL),
          m_pLastSearchReply(NULL) {
    setupUi(this);

    connect(btnSearch, SIGNAL(clicked()),
            this, SLOT(slotSearch()));
    connect(btnApply, SIGNAL(clicked()),
            this, SLOT(slotApply()));
    connect(btnClose, SIGNAL(clicked()),
            this, SLOT(slotClose()));
    connect(btnPrev, SIGNAL(clicked()),
            this, SIGNAL(previous()));
    connect(btnNext, SIGNAL(clicked()),
            this, SIGNAL(next()));

    btnSearch->setCheckable(true);

    coverView->horizontalHeader()->setStretchLastSection(true);
    coverView->horizontalHeader()->setVisible(false);
    coverView->verticalHeader()->setVisible(false);
    coverView->setAlternatingRowColors(false);
    coverView->setSelectionBehavior(QAbstractItemView::SelectItems);
    coverView->setSelectionMode(QAbstractItemView::NoSelection);
    coverView->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

DlgCoverArtFetcher::~DlgCoverArtFetcher() {
    qDebug() << "~DlgCoverArtFetcher()";
}

void DlgCoverArtFetcher::init(const TrackPointer track) {
    abortSearch();
    m_downloadQueue.clear();
    m_searchresults.clear();
    coverView->setModel(NULL);
    setStatusOfSearchBtn(false);

    if (track->getAlbum().isEmpty() && track->getArtist().isEmpty()) {
        txtAlbum->setText(track->getTitle());
    } else {
        txtAlbum->setText(track->getAlbum());
        txtArtist->setText(track->getArtist());
    }
}

void DlgCoverArtFetcher::setStatus(Status status) {
    QString txt;
    switch (status) {
        case READY:
            txt = tr("ready!");
            break;
        case NOTFOUND:
            txt = tr("No images were found!");
            break;
        case SEARCHING:
            txt = tr("searching...");
            break;
    }
    txtStatus->setText(txt);
}

void DlgCoverArtFetcher::setStatusOfSearchBtn(bool isSearching) {
    btnSearch->setChecked(isSearching);
    if (isSearching) {
        setStatus(SEARCHING);
        btnSearch->setText(tr("Abort"));
        txtArtist->setEnabled(false);
        txtAlbum->setEnabled(false);
    } else {
        setStatus(READY);
        btnSearch->setText(tr("Search"));
        txtArtist->setEnabled(true);
        txtAlbum->setEnabled(true);
    }
}

void DlgCoverArtFetcher::abortSearch() {
    if (m_pLastSearchReply != NULL) {
        m_pLastSearchReply->abort();
        m_pLastSearchReply->deleteLater();
    }
    if (m_pLastDownloadReply != NULL) {
        m_pLastDownloadReply->abort();
        m_pLastDownloadReply->deleteLater();
    }
}

void DlgCoverArtFetcher::slotClose() {
    abortSearch();
    close();
}

void DlgCoverArtFetcher::slotSearch() {
    if (btnSearch->isChecked()) {
        coverView->setModel(NULL);
        setStatusOfSearchBtn(true);

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
                this, SLOT(slotSearchFinished()));
    } else {
        abortSearch();
        setStatusOfSearchBtn(false);
    }
}

void DlgCoverArtFetcher::slotSearchFinished() {
    m_downloadQueue.clear();
    m_searchresults.clear();

    if (m_pLastSearchReply->error() != QNetworkReply::NoError) {
        m_pLastSearchReply->deleteLater();
        m_pLastSearchReply = NULL;
        setStatusOfSearchBtn(false);
        return;
    }

    QUrl url;
    SearchResult res;
    QXmlStreamReader xml(m_pLastSearchReply->readAll());
    while(!xml.atEnd() && !xml.hasError())
    {
        xml.readNext();
        if(xml.name() == "album") {
            parseAlbum(xml, res.album, res.artist, url);
            if (url.isValid()) {
                m_downloadQueue.append(url);
                m_searchresults.append(res);
            }
        }
    }

    m_pLastSearchReply->deleteLater();
    m_pLastSearchReply = NULL;
    downloadNextCover();
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
        showResults();
        return;
    }

    m_pLastDownloadReply = m_pNetworkManager->get(
                QNetworkRequest(m_downloadQueue.first()));

    connect(m_pLastDownloadReply, SIGNAL(finished()),
            this, SLOT(slotDownloadFinished()));
}

void DlgCoverArtFetcher::slotDownloadFinished() {
    m_downloadQueue.removeFirst();

    if (m_pLastDownloadReply->error() != QNetworkReply::NoError) {
        m_pLastDownloadReply->deleteLater();
        m_pLastDownloadReply = NULL;
        m_searchresults.removeFirst();
        downloadNextCover();
        return;
    }

    QPixmap pixmap;
    if (pixmap.loadFromData(m_pLastDownloadReply->readAll())) {
        m_searchresults.first().cover = pixmap;
        m_searchresults.move(0, m_searchresults.size() - 1);
    } else {
        m_searchresults.removeFirst();
    }

    m_pLastDownloadReply->deleteLater();
    m_pLastDownloadReply = NULL;
    downloadNextCover();
}

void DlgCoverArtFetcher::showResults() {
    if (m_searchresults.isEmpty()) {
        coverView->setModel(NULL);
        setStatusOfSearchBtn(false);
        return;
    }

    // TODO: The whole COVERVIEW must be improved!
    //       it is just a sketch to make us able to test the downloads.
    const int COLUMNCOUNT = 5;
    const int ROWCOUNT = m_searchresults.size() / COLUMNCOUNT + 1;
    QStandardItemModel* model = new QStandardItemModel(ROWCOUNT, COLUMNCOUNT, this);
    coverView->setModel(model);

    int index = 0;
    for (int row=0; row<ROWCOUNT; row++) {
        coverView->setRowHeight(row, 130);
        for (int column=0; column<COLUMNCOUNT; column++) {
            if (index >= m_searchresults.size()) {
                setStatusOfSearchBtn(false);
                return;
            }

            QString title = m_searchresults.at(index).album % " - " %
                            m_searchresults.at(index).artist;

            QToolButton* button = new QToolButton(this);
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            button->setAutoRaise(true);
            button->setIcon(m_searchresults.at(index).cover);
            button->setIconSize(QSize(100,100));
            button->setText(title);

            coverView->setIndexWidget(coverView->model()->index(row, column), button);
            index++;
        }
    }
}

void DlgCoverArtFetcher::slotApply() {
    // TODO
}
