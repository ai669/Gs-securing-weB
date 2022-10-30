#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "clientmodel.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "bitcoingui.h"

#include "qrcodedialog.h"

#include "init.h"
#include "rpcserver.h"
#include "rpcclient.h"
#include "kernel.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QScrollArea>
#include <QScroller>
#include <QSettings>
#include <QTimer>
#include <QMovie>
#include <QPixmap>

#include <QMessageBox>

#define DECORATION_SIZE 64
#define ICON_OFFSET 16
#define NUM_ITEMS 6

int64_t balAmount = 0;

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(WayaWolfCoinUnits::WW)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(qVariantCanConvert<QColor>(value))
        {
            foreground = qvariant_cast<QColor>(value);
        }

        balAmount = amount;

        painter->setPen(fUseDarkTheme ? QColor(255, 255, 255) : foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(fUseDarkTheme ? QColor(255, 255, 255) : foreground);
        QString amountText = WayaWolfCoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(fUseDarkTheme ? QColor(96, 101, 110) : option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentStake(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchOnlyStake(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions_2->setItemDelegate(txdelegate);
    ui->listTransactions_2->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions_2->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions_2->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listTransactions_2->setMinimumWidth(300);

    connect(ui->listTransactions_2, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelTransactionsStatus->setText(tr("Synchronizing... Please wait."));
    QMovie *SYNCmovie = new QMovie(":/gifs/syncgif");
    ui->syncLabel->setMovie(SYNCmovie);
    SYNCmovie->stop();// Initially set stopped
    ui->syncLabel->setVisible(true);

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    // Always show these labels (Sync Status)
    ui->labelTransactionsStatus->setVisible(true);
    //ui->syncstatusGIF->setVisible(true);

    if (fUseDarkTheme)
    {
        const char* whiteLabelQSS = "QLabel { color: rgb(255,255,255); }";
        ui->labelBalance->setStyleSheet(whiteLabelQSS);
        ui->labelStake->setStyleSheet(whiteLabelQSS);
        ui->labelUnconfirmed->setStyleSheet(whiteLabelQSS);
        ui->labelImmature->setStyleSheet(whiteLabelQSS);
        ui->labelTotal->setStyleSheet(whiteLabelQSS);
    }
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& stake, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& watchOnlyBalance, const CAmount& watchOnlyStake, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchOnlyStake = watchOnlyStake;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    ui->labelBalance->setText(WayaWolfCoinUnits::formatWithUnit(nDisplayUnit, balance));
    ui->labelStake->setText(WayaWolfCoinUnits::formatWithUnit(nDisplayUnit, stake));
    ui->labelUnconfirmed->setText(WayaWolfCoinUnits::formatWithUnit(nDisplayUnit, unconfirmedBalance));
    ui->labelImmature->setText(WayaWolfCoinUnits::formatWithUnit(nDisplayUnit, immatureBalance));
    ui->labelTotal->setText(WayaWolfCoinUnits::formatWithUnit(nDisplayUnit, balance + stake + unconfirmedBalance + immatureBalance));
    //ui->labelWatchAvailable->setText(WayaWolfCoinUnits::floorWithUnit(nDisplayUnit, watchOnlyBalance));
    //ui->labelWatchStake->setText(WayaWolfCoinUnits::floorWithUnit(nDisplayUnit, watchOnlyStake));
    //ui->labelWatchPending->setText(WayaWolfCoinUnits::floorWithUnit(nDisplayUnit, watchUnconfBalance));
    //ui->labelWatchImmature->setText(WayaWolfCoinUnits::floorWithUnit(nDisplayUnit, watchImmatureBalance));
    //ui->labelWatchTotal->setText(WayaWolfCoinUnits::floorWithUnit(nDisplayUnit, watchOnlyBalance + watchOnlyStake + watchUnconfBalance + watchImmatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = true;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the