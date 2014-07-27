#include <QStandardItemModel>
#include <QStringBuilder>
#include <QtDebug>

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
    connect(btnCancel, SIGNAL(clicked()),
            this, SLOT(slotCancel()));
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
    qDebug() << "~DlgCoverArtDownload()";
}

void DlgCoverArtFetcher::init(const TrackPointer track) {
    abortSearch();
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

void DlgCoverArtFetcher::setStatusOfSearchBtn(bool isSearching) {
    btnSearch->setChecked(isSearching);
    if (isSearching) {
        btnSearch->setText("Abort");
        txtArtist->setEnabled(false);
        txtAlbum->setEnabled(false);
    } else {
        btnSearch->setText("Search");
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

void DlgCoverArtFetcher::slotCancel() {
    abortSearch();
    close();
}

void DlgCoverArtFetcher::slotSearch() {
    if (btnSearch->isChecked()) {
        setStatusOfSearchBtn(true);

        QUrl url;
        url.setScheme("http");
        url.setHost("ws.audioscrobbler.com");
        url.setPath("/2.0/");
        url.addQueryItem("method", "album.search");
        url.addQueryItem("album", txtAlbum->text() % " " % txtArtist->text());
        url.addQueryItem("api_key", APIKEY_LASTFM);

        QNetworkRequest req(url);
        m_pLastSearchReply = m_pNetworkManager->get(req);
        connect(m_pLastSearchReply, SIGNAL(finished()), this, SLOT(slotSearchFinished()));
    } else {
        abortSearch();
        setStatusOfSearchBtn(false);
    }
}

void DlgCoverArtFetcher::slotSearchFinished() {
    m_searchresults.clear();
    if (m_pLastSearchReply->error() != QNetworkReply::NoError) {
        m_pLastSearchReply->deleteLater();
        m_pLastSearchReply = NULL;
        setStatusOfSearchBtn(false);
        return;
    }

    QXmlStreamReader xml(m_pLastSearchReply->readAll());
    while(!xml.atEnd() && !xml.hasError())
    {
        xml.readNext();
        if(xml.name() == "album") {
            parseAlbum(xml);
        }
    }
    m_pLastSearchReply->deleteLater();
    m_pLastSearchReply = NULL;
    downloadNextCover();
}

void DlgCoverArtFetcher::parseAlbum(QXmlStreamReader& xml) {
    if (xml.name() != "album") {
        return;
    }
    QString cover_url;
    SearchResult result;
    while(!xml.atEnd() && !xml.hasError())
    {
        xml.readNext();
        const QStringRef &name = xml.name();

        if (name == "album") {
            break;
        }

        if (name == "name") {
            result.album = xml.readElementText();
        } else if (name == "artist") {
            result.artist = xml.readElementText();
        } else if (name == "image") {
            if (xml.attributes().value("size").toString() == "extralarge") {
                cover_url = xml.readElementText();
            }
        }
    }

    if (!cover_url.isEmpty()) {
        m_searchresults.insert(cover_url, result);
    }
}

void DlgCoverArtFetcher::downloadNextCover() {
    if (m_searchresults.isEmpty()) {
        return;
    }

    QString nextUrl;
    QMapIterator<QString, SearchResult> it(m_searchresults);
    while (it.hasNext()) {
        it.next();
        if (it.value().cover.isNull()) {
            nextUrl = it.key();
            break;
        }
    }

    if (nextUrl.isEmpty()) {
        showResults();
        return;
    }

    m_pLastDownloadReply = m_pNetworkManager->get(QNetworkRequest(QUrl(nextUrl)));
    QString reqUrl = m_pLastDownloadReply->url().toString();
    if (nextUrl != reqUrl) {
        m_searchresults.insert(reqUrl, m_searchresults.take(nextUrl));
    }
    connect(m_pLastDownloadReply, SIGNAL(finished()),
            this, SLOT(slotDownloadFinished()));
}

void DlgCoverArtFetcher::slotDownloadFinished() {
    if (m_pLastDownloadReply->error() != QNetworkReply::NoError) {
        m_pLastDownloadReply->deleteLater();
        m_pLastDownloadReply = NULL;
        setStatusOfSearchBtn(false);
        return;
    }

    QPixmap pixmap;
    QString reqUrl = m_pLastDownloadReply->url().toString();
    if (pixmap.loadFromData(m_pLastDownloadReply->readAll())) {
        SearchResult res = m_searchresults.value(reqUrl);
        res.cover = pixmap;
        m_searchresults.insert(reqUrl, res);
    } else {
        m_searchresults.remove(reqUrl);
    }

    m_pLastDownloadReply->deleteLater();
    m_pLastDownloadReply = NULL;
    downloadNextCover();
}

void DlgCoverArtFetcher::showResults() {
    setStatusOfSearchBtn(false);

    if (m_searchresults.isEmpty()) {
        coverView->setModel(NULL);
        return;
    }

    const int COLUMNCOUNT = 5;
    const int ROWCOUNT = m_searchresults.size() / COLUMNCOUNT + 1;
    QStandardItemModel* model = new QStandardItemModel(ROWCOUNT, COLUMNCOUNT, this);
    coverView->setModel(model);

    int index = 0;
    QList<SearchResult> results = m_searchresults.values();
    for (int row=0; row<ROWCOUNT; row++) {
        coverView->setRowHeight(row, 100);
        for (int column=0; column<COLUMNCOUNT; column++) {
            if (index >= results.size()) {
                return;
            }

            QPushButton* btn = new QPushButton("");
            btn->setFlat(true);
            btn->setIcon(results[index].cover);
            btn->setIconSize(QSize(100,100));
            coverView->setIndexWidget(coverView->model()->index(row, column), btn);
            index++;
        }
    }
}
