#ifndef DLGCOVERARTFETCHER_H
#define DLGCOVERARTFETCHER_H

#include <QDialog>

#include "trackinfoobject.h"
#include "ui_dlgcoverartfetcher.h"

class DlgCoverArtFetcher : public QDialog, public Ui::DlgCoverArtFetcher {
    Q_OBJECT
  public:
    DlgCoverArtFetcher(QWidget *parent);
    virtual ~DlgCoverArtFetcher();

    void init(const TrackPointer track);

  private:
    TrackPointer m_track;
};

#endif // DLGCOVERARTFETCHER_H
