#include <QStringBuilder>
#include <QtDebug>

#include "dlgcoverartfetcher.h"

DlgCoverArtFetcher::DlgCoverArtFetcher(QWidget *parent)
        : QDialog(parent),
          m_track(NULL) {
    setupUi(this);

    connect(btnCancel, SIGNAL(clicked()),
            this, SLOT(slotCancel()));
    connect(btnSearch, SIGNAL(clicked()),
            this, SLOT(slotSearch()));
}

DlgCoverArtFetcher::~DlgCoverArtFetcher() {
    qDebug() << "~DlgCoverArtDownload()";
}

void DlgCoverArtFetcher::init(const TrackPointer track) {
    txtAlbum->setText(track->getAlbum());
    txtArtist->setText(track->getArtist());
    m_sLastRequestedAlbum.clear();
    m_sLastRequestedArtist.clear();
    m_searchresults.clear();
}

void DlgCoverArtFetcher::slotCancel() {
    close();
}

void DlgCoverArtFetcher::slotSearch() {
    btnSearch->setEnabled(false);
    m_sLastRequestedAlbum = txtAlbum->text();
    m_sLastRequestedArtist = txtArtist->text();

    QUrl url;
    url.setScheme("http");
    url.setHost("ws.audioscrobbler.com");
    url.setPath("/2.0/");
    url.addQueryItem("method", "album.search");
    url.addQueryItem("album", m_sLastRequestedAlbum % " " % m_sLastRequestedArtist);
    url.addQueryItem("api_key", APIKEY_LASTFM);

    QNetworkRequest req(url);
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    m_pLastReply = manager->get(req);
    connect(m_pLastReply, SIGNAL(finished()), this, SLOT(slotSearchFinished()));
}

void DlgCoverArtFetcher::slotSearchFinished() {
    m_searchresults.clear();
    m_pLastReply->deleteLater();

    QXmlStreamReader xml(m_pLastReply->readAll());
    while(!xml.atEnd() && !xml.hasError())
    {
        xml.readNext();
        if(xml.name() == "album") {
            SearchResult result = parseAlbum(xml);
            if (result.cover_url.isValid()) {
                m_searchresults.append(result);
            }
        }
    }
    qDebug() << "found: "  << m_searchresults.size();
    btnSearch->setEnabled(true);
}

DlgCoverArtFetcher::SearchResult DlgCoverArtFetcher::parseAlbum(
                                                QXmlStreamReader& xml) {
    if (xml.name() != "album") {
        return SearchResult();
    }
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
                result.cover_url = xml.readElementText();
            }
        }
    }

    return result;
}
