#include <QtDebug>

#include "dlgcoverartfetcher.h"

DlgCoverArtFetcher::DlgCoverArtFetcher(QWidget *parent)
        : QDialog(parent),
          m_track(NULL) {
    setupUi(this);
}

DlgCoverArtFetcher::~DlgCoverArtFetcher() {
    qDebug() << "~DlgCoverArtDownload()";
}

void DlgCoverArtFetcher::init(const TrackPointer track) {
}
