#include <QStringBuilder>
#include <QtDebug>
#include <QToolButton>

#include "dlgcoverartfetcher.h"
#include "library/coverartcache.h"

DlgCoverArtFetcher::DlgCoverArtFetcher(QWidget *parent)
        : QDialog(parent),
          m_track(NULL),
          m_cells(NULL),
          m_model(NULL),
          m_pNetworkManager(new QNetworkAccessManager(this)),
          m_pLastDownloadReply(NULL),
          m_pLastSearchReply(NULL),
          m_kCellSize(100, 150),
          m_kCoverSize(100, 100) {
    setupUi(this);

    connect(btnSearch, SIGNAL(clicked()),
            this, SLOT(slotSearch()));
    connect(btnClose, SIGNAL(clicked()),
            this, SLOT(slotClose()));
    connect(btnPrev, SIGNAL(clicked()),
            this, SIGNAL(previous()));
    connect(btnNext, SIGNAL(clicked()),
            this, SIGNAL(next()));

    btnSearch->setCheckable(true);

    coverView->horizontalHeader()->setVisible(false);
    coverView->verticalHeader()->setVisible(false);
    coverView->setAlternatingRowColors(false);
    coverView->setSelectionBehavior(QAbstractItemView::SelectItems);
    coverView->setSelectionMode(QAbstractItemView::NoSelection);
    coverView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    coverView->setShowGrid(false);
}

DlgCoverArtFetcher::~DlgCoverArtFetcher() {
    qDebug() << "~DlgCoverArtFetcher()";
}

void DlgCoverArtFetcher::init(const TrackPointer track) {
    abortSearch();
    initCoverView();
    m_track = track;

    if (track->getAlbum().isEmpty() && track->getArtist().isEmpty()) {
        txtAlbum->setText(track->getTitle());
    } else {
        txtAlbum->setText(track->getAlbum());
        txtArtist->setText(track->getArtist());
    }
}

void DlgCoverArtFetcher::initCoverView() {
    m_downloadQueue.clear();
    m_model = new QStandardItemModel(this);
    coverView->setModel(m_model);
    delete m_cells;
    m_cells = new QButtonGroup(this);
    connect(m_cells, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(slotApply(QAbstractButton*)));
}

void DlgCoverArtFetcher::setStatus(Status status) {
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
    btnSearch->setText(btnText);
    btnSearch->setChecked(!enableFields);
    txtArtist->setEnabled(enableFields);
    txtAlbum->setEnabled(enableFields);
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

void DlgCoverArtFetcher::slotClose() {
    abortSearch();
    close();
}

void DlgCoverArtFetcher::slotSearch() {
    if (!btnSearch->isChecked()) {
        abortSearch();
        return;
    }

    initCoverView();
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
            this, SLOT(slotSearchFinished()));
}

void DlgCoverArtFetcher::slotSearchFinished() {
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
        setStatus(READY);
        return;
    }

    m_pLastDownloadReply = m_pNetworkManager->get(
                QNetworkRequest(m_downloadQueue.first().url));

    connect(m_pLastDownloadReply, SIGNAL(finished()),
            this, SLOT(slotDownloadFinished()));
}

void DlgCoverArtFetcher::slotDownloadFinished() {
    if (m_pLastDownloadReply->error() == QNetworkReply::NoError) {
        QPixmap pixmap;
        if (pixmap.loadFromData(m_pLastDownloadReply->readAll())) {
            showResult(m_downloadQueue.first(), pixmap);
        }
    }

    m_pLastDownloadReply->deleteLater();
    m_pLastDownloadReply = NULL;
    m_downloadQueue.removeFirst();
    downloadNextCover();
}

void DlgCoverArtFetcher::showResult(SearchResult res, QPixmap pixmap) {
    QToolButton* cell = new QToolButton(this);
    cell->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    cell->setAutoRaise(true);
    cell->setIcon(pixmap);
    cell->setIconSize(m_kCoverSize);

    m_cells->addButton(cell);

    res.album.truncate(16);
    res.artist.truncate(16);
    cell->setText(res.album % "\n" % res.artist);
    cell->setStyleSheet("font: 7pt 'Sans Serif'");

    bool newRow;
    int row, column;
    getNextPosition(newRow, row, column);

    if (newRow) {
        m_model->insertRow(row, new QStandardItem(""));
    } else {
        m_model->setItem(row, column, new QStandardItem(""));
    }

    coverView->setModel(m_model);
    coverView->setColumnWidth(column, m_kCellSize.width());
    coverView->setRowHeight(row, m_kCellSize.height());

    coverView->setIndexWidget(coverView->model()->index(row, column), cell);
}

void DlgCoverArtFetcher::getNextPosition(bool& newRow, int& row, int& column) {
    int columnCount = m_model->columnCount();
    int rowCount = m_model->rowCount();
    int maxColumnCount = (float) coverView->size().width() / m_kCellSize.width();

    bool found = false;
    if (columnCount * rowCount == 0) { // first result
        newRow = true;
        column = 0;
        row = 0;
        found = true;
    }

    for (int r = 0; r < rowCount && !found; r++) {
        for (int c = 0; c < columnCount && !found; c++) {
            if (m_model->data(m_model->index(r, c)).isNull()) {
                newRow = false;
                row = r;
                column = c;
                found = true;
            }
        }
    }

    if (!found) {
        if (columnCount < maxColumnCount) {
            m_model->setColumnCount(columnCount + 1);
            getNextPosition(newRow, row, column);
        } else {
            newRow = true;
            column = 0;
            row = rowCount;
        }
    }
}

void DlgCoverArtFetcher::slotApply(QAbstractButton* cell) {
    if (m_track == NULL) {
        return;
    }

    QString trackPath = m_track->getDirectory();
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
    if (cell->icon().pixmap(1000).save(newCoverLocation)) {
        res = CoverArtCache::instance()->changeCoverArt(m_track->getId(),
                                                        newCoverLocation);
    }

    if (!res) {
        QMessageBox::warning(this, tr("Cover Art"),
                             tr("Could not change the cover art!"));
        return;
    }

    slotClose();
}
