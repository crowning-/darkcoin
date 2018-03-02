// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qrdialog.h"
#include "ui_qrdialog.h"

#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"

#include <QClipboard>
#include <QDrag>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#if QT_VERSION < 0x050000
#include <QUrl>
#endif

#if defined(HAVE_CONFIG_H)
#include "config/dash-config.h" /* for USE_QRCODE */
#endif

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

QRGeneralImageWidget::QRGeneralImageWidget(QWidget *parent):
    QLabel(parent), contextMenu(0)
{
    contextMenu = new QMenu(this);
    QAction *saveImageAction = new QAction(tr("&Save Image..."), this);
    connect(saveImageAction, SIGNAL(triggered()), this, SLOT(saveImage()));
    contextMenu->addAction(saveImageAction);
    QAction *copyImageAction = new QAction(tr("&Copy Image"), this);
    connect(copyImageAction, SIGNAL(triggered()), this, SLOT(copyImage()));
    contextMenu->addAction(copyImageAction);
}

QImage QRGeneralImageWidget::exportImage()
{
    if(!pixmap())
        return QImage();
    return pixmap()->toImage().scaled(EXPORT_IMAGE_SIZE, EXPORT_IMAGE_SIZE);
}

void QRGeneralImageWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && pixmap())
    {
        event->accept();
        QMimeData *mimeData = new QMimeData;
        mimeData->setImageData(exportImage());

        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec();
    } else {
        QLabel::mousePressEvent(event);
    }
}

void QRGeneralImageWidget::saveImage()
{
    if(!pixmap())
        return;
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Image (*.png)"), NULL);
    if (!fn.isEmpty())
    {
        exportImage().save(fn);
    }
}

void QRGeneralImageWidget::copyImage()
{
    if(!pixmap())
        return;
    QApplication::clipboard()->setImage(exportImage());
}

void QRGeneralImageWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if(!pixmap())
        return;
    contextMenu->exec(event->globalPos());
}

QRDialog::QRDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QRDialog),
    model(0)
{
    ui->setupUi(this);

#ifndef USE_QRCODE
    ui->button_saveImage->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    connect(ui->button_saveImage, SIGNAL(clicked()), ui->lblQRCode, SLOT(saveImage()));
}

QRDialog::~QRDialog()
{
    delete ui;
}

void QRDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if (model)
        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(update()));

    // update the display unit if necessary
    update();
}

void QRDialog::setInfo(const std::string &strMNPrivKey, const std::string &strMNAlias)
{
    this->strMNPrivKey = strMNPrivKey;
    this->strMNAlias = strMNAlias;
    update();
}

void QRDialog::update()
{
    if(!model)
        return;

    QString strMNKeyInfo = QString::fromStdString(strMNPrivKey);
    QString strMNInfo = QString::fromStdString(strMNAlias);

    setWindowTitle(tr("Private Key for Masternode %1").arg(strMNInfo));
    ui->button_saveImage->setEnabled(false);
    
    // Create dialog text
    QString html;
    html += "<html><font face='verdana, arial, helvetica, sans-serif'>";
    html += "<b>" + tr("Masternode Alias") + ": </b>" + GUIUtil::HtmlEscape(strMNAlias) + "<br>";
    html += "<b>" + tr("Private Key") +      ": </b>" + GUIUtil::HtmlEscape(strMNKeyInfo) + "<br>";
    ui->outUri->setText(html);

#ifdef USE_QRCODE
    // Create QR-code
    ui->lblQRCode->setText("");
    if(!strMNKeyInfo.isEmpty())
    {
        QRcode *code = QRcode_encodeString(strMNKeyInfo.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!code)
        {
            ui->lblQRCode->setText(tr("Private Key into QR Code."));
            return;
        }
        QImage myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
        myImage.fill(0xffffff);
        unsigned char *p = code->data;
        for (int y = 0; y < code->width; y++)
        {
            for (int x = 0; x < code->width; x++)
            {
                myImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                p++;
            }
        }
        QRcode_free(code);

        ui->lblQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(300, 300));
        ui->button_saveImage->setEnabled(true);
    }
#endif
}

void QRDialog::on_button_copyAlias_clicked()
{
    GUIUtil::setClipboard(QString::fromStdString(strMNAlias));
}

void QRDialog::on_button_copyKey_clicked()
{
    GUIUtil::setClipboard(QString::fromStdString(strMNPrivKey));
}