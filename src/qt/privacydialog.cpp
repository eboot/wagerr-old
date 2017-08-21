// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "privacydialog.h"
#include "ui_privacydialog.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "coincontroldialog.h"
#include "libzerocoin/Denominations.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "coincontrol.h"

#include <QClipboard>

PrivacyDialog::PrivacyDialog(QWidget* parent) : QDialog(parent),
                                                          ui(new Ui::PrivacyDialog),
                                                          walletModel(0),
                                                          currentBalance(-1)
{
    nDisplayUnit = 0; // just make sure it's not unitialized
    ui->setupUi(this);

    // "Spending 999999 zPIV ought to be enough for anybody." - Bill Gates, 2017
    ui->zPIVpayAmount->setValidator( new QIntValidator(0, 999999, this) );
    ui->labelMintAmountValue->setValidator( new QIntValidator(0, 999999, this) );

    // Default texts for (mini-) coincontrol
    ui->labelCoinControlQuantity->setText (tr("Coins automatically selected"));
    ui->labelCoinControlAmount->setText (tr("Coins automatically selected"));
    ui->labelzPIVSyncStatus->setText("(" + tr("out of sync") + ")");

    // Sunken frame for minting messages
    ui->labelMintStatus->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    ui->labelMintStatus->setLineWidth (2);
    ui->labelMintStatus->setMidLineWidth (2);

    // Coin Control signals
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));

    // Coin Control: clipboard actions
    QAction* clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction* clipboardAmountAction = new QAction(tr("Copy amount"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);

    // Start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

PrivacyDialog::~PrivacyDialog()
{
    delete ui;
}

void PrivacyDialog::setModel(WalletModel* walletModel)
{
    this->walletModel = walletModel;

    if (walletModel && walletModel->getOptionsModel()) {
        // Keep up to date with wallet
        setBalance(walletModel->getBalance(),         walletModel->getUnconfirmedBalance(), walletModel->getImmatureBalance(), 
                   walletModel->getZerocoinBalance(), walletModel->getWatchBalance(),       walletModel->getWatchUnconfirmedBalance(), 
                   walletModel->getWatchImmatureBalance());
        
        connect(walletModel, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this, 
                               SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));
    }
}

void PrivacyDialog::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void PrivacyDialog::on_addressBookButton_clicked()
{
    if (!walletModel)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(walletModel->getAddressTableModel());
    if (dlg.exec()) {
        ui->payTo->setText(dlg.getReturnValue());
        ui->zPIVpayAmount->setFocus();
    }
}

void PrivacyDialog::on_pushButtonMintzPIV_clicked()
{
    if (!walletModel || !walletModel->getOptionsModel())
        return;

    // Reset message text
    ui->labelMintStatus->setText(tr("Mint Status: Okay"));
    
    // Wallet must be unlocked for minting
    if (pwalletMain->IsLocked()){
        ui->labelMintStatus->setText(tr("Error: your wallet is locked. Please enter the wallet passphrase first."));
        return;
    }

    QString sAmount = ui->labelMintAmountValue->text();
    CAmount nAmount = sAmount.toInt() * COIN;

    // Minting amount must be > 0
    if(nAmount <= 0){
        ui->labelMintStatus->setText(tr("Message: Enter an amount > 0."));
        return;
    }

    ui->labelMintStatus->setText(tr("Minting ") + ui->labelMintAmountValue->text() + " zPIV...");
    ui->labelMintStatus->repaint ();
    
    int64_t nTime = GetTimeMillis();
    
    CWalletTx wtx;
    vector<CZerocoinMint> vMints;
    string strError = pwalletMain->MintZerocoin(nAmount, wtx, vMints, CoinControlDialog::coinControl);
    
    // Return if something went wrong during minting
    if (strError != ""){
        ui->labelMintStatus->setText(QString::fromStdString(strError));
        return;
    }

    int64_t nDuration = GetTimeMillis() - nTime;
    
    // Minting successfully finished. Show some stats for entertainment.
    QString strStatsHeader = tr("Successfully minted ") + ui->labelMintAmountValue->text() + tr(" zPIV in ") + 
                             QString::number(nDuration) + tr(" ms. Used denominations:\n");
    QString strStats = "";
    ui->labelMintStatus->setText(strStatsHeader);

    for (CZerocoinMint mint : vMints) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        strStats = strStats + QString::number(mint.GetDenomination()) + " ";
        ui->labelMintStatus->setText(strStatsHeader + strStats);
        ui->labelMintStatus->repaint ();
        
    }

    // Available balance isn't always updated, so force it.
    setBalance(walletModel->getBalance(),         walletModel->getUnconfirmedBalance(), walletModel->getImmatureBalance(), 
               walletModel->getZerocoinBalance(), walletModel->getWatchBalance(),       walletModel->getWatchUnconfirmedBalance(), 
               walletModel->getWatchImmatureBalance());
    coinControlUpdateLabels();

    return;
}

void PrivacyDialog::on_pushButtonMintReset_clicked()
{
    if (!walletModel || !walletModel->getOptionsModel())
        return;

    QMessageBox::warning(this, tr("Reset Zerocoin"),
                tr("Test for Reset"),
                QMessageBox::Ok, QMessageBox::Ok);
            return;

        return;
}

void PrivacyDialog::on_pushButtonSpendzPIV_clicked()
{
    if (!walletModel || !walletModel->getOptionsModel() || !pwalletMain)
        return;

    CBitcoinAddress address(ui->payTo->text().toStdString());
    if (!address.IsValid()) {
        QMessageBox::warning(this, tr("Spend Zerocoin"), tr("Invalid Pivx Address"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    //grab as a double, increase by COIN and cutoff any remainder by assigning it as int64_t/CAmount
    double dAmount = ui->zPIVpayAmount->text().toDouble();
    CAmount nAmount = dAmount * COIN;
    if (!MoneyRange(nAmount)) {
        QMessageBox::warning(this, tr("Spend Zerocoin"), tr("Invalid Send Amount"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    int nSecurityLevel = ui->securityLevel->value();
    bool fMintChange = ui->checkBoxMintChange->isChecked();
    CWalletTx wtxNew;
    vector<CZerocoinMint> vMintsSelected;
    vector<CZerocoinSpend> vSpends;

    // attempt to spend the zPiv
    ui->labelMintStatus->setText(tr("Spending Zerocoin. Computationally expensive, please be patient."));
    ui->labelMintStatus->repaint();
    string strError = pwalletMain->SpendZerocoin(nAmount, nSecurityLevel, wtxNew, vSpends, vMintsSelected, fMintChange, &address);

    if (strError != "") {
        QMessageBox::warning(this, tr("Spend Zerocoin"), tr(strError.c_str()), QMessageBox::Ok, QMessageBox::Ok);
        ui->labelMintStatus->setText(tr("Spend Zerocoin Failed!"));
        ui->labelMintStatus->repaint();
        return;
    }

    QString strStats = "";
    CAmount nValueIn = 0;
    int nCount = 0;
    for (CZerocoinSpend spend : vSpends) {
        strStats += tr("zPiv Spend #: ") + QString::number(nCount) + ", ";
        strStats += tr("denomination: ") + QString::number(spend.GetDenomination()) + ", ";
        strStats += tr("serial: ") + spend.GetSerial().ToString().c_str() + "\n";
        nValueIn += libzerocoin::ZerocoinDenominationToAmount(spend.GetDenomination());
    }

    CAmount nValueOut = 0;
    for (const CTxOut& txout: wtxNew.vout) {
        strStats += tr("value out: ") + QString::number(txout.nValue/COIN) + " Piv, ";
        nValueOut += txout.nValue;

        strStats += tr("address: ");
        CTxDestination dest;
        if(txout.scriptPubKey.IsZerocoinMint())
            strStats += tr("zPiv Mint");
        else if(ExtractDestination(txout.scriptPubKey, dest))
            strStats += tr(CBitcoinAddress(dest).ToString().c_str());
        strStats += "\n";
    }

    QString strReturn;
    strReturn += tr("txid: ") + wtxNew.GetHash().ToString().c_str() + "\n";
    strReturn += tr("fee: ") + QString::number((nValueIn-nValueOut)/COIN) + "\n";
    strReturn += strStats;

    ui->labelMintStatus->setText(strReturn);
    ui->labelMintStatus->repaint();
}

void PrivacyDialog::on_payTo_textChanged(const QString& address)
{
    updateLabel(address);
}

// Coin Control: copy label "Quantity" to clipboard
void PrivacyDialog::coinControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void PrivacyDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: button inputs -> show actual coin control dialog
void PrivacyDialog::coinControlButtonClicked()
{
    CoinControlDialog dlg;
    dlg.setModel(walletModel);
    dlg.exec();
    coinControlUpdateLabels();
}

// Coin Control: update labels
void PrivacyDialog::coinControlUpdateLabels()
{
    if (!walletModel || !walletModel->getOptionsModel() || !walletModel->getOptionsModel()->getCoinControlFeatures())
        return;

     // set pay amounts
    CoinControlDialog::payAmounts.clear();

    if (CoinControlDialog::coinControl->HasSelected()) {
        // Actual coin control calculation
        CoinControlDialog::updateLabels(walletModel, this);
    } else {
        ui->labelCoinControlQuantity->setText (tr("Coins automatically selected"));
        ui->labelCoinControlAmount->setText (tr("Coins automatically selected"));
    }
}

bool PrivacyDialog::updateLabel(const QString& address)
{
    if (!walletModel)
        return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = walletModel->getAddressTableModel()->labelForAddress(address);
    if (!associatedLabel.isEmpty()) {
        ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}

void PrivacyDialog::setBalance(const CAmount& balance,         const CAmount& unconfirmedBalance, const CAmount& immatureBalance, 
                               const CAmount& zerocoinBalance, const CAmount& watchOnlyBalance,   const CAmount& watchUnconfBalance, 
                               const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentZerocoinBalance = zerocoinBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;

    CWalletDB walletdb(pwalletMain->strWalletFile);
    list<CZerocoinMint> listPubCoin = walletdb.ListMintedCoins(true);
 
    std::map<libzerocoin::CoinDenomination, CAmount> spread;
    for (const auto& denom : libzerocoin::zerocoinDenomList){
        spread.insert(std::pair<libzerocoin::CoinDenomination, CAmount>(denom, 0));
    }
    for (auto& mint : listPubCoin){
        spread.at(mint.GetDenomination())++;
    }
    
    int64_t nCoins = 0;
    for (const auto& m : libzerocoin::zerocoinDenomList) {
        nCoins = libzerocoin::ZerocoinDenominationToInt(m);
        switch (nCoins) {
            case libzerocoin::CoinDenomination::ZQ_ONE: 
                ui->labelzDenom1Amount->setText (QString::number(spread.at(m)));
                break;
            case libzerocoin::CoinDenomination::ZQ_FIVE:
                ui->labelzDenom2Amount->setText (QString::number(spread.at(m)));
                break;
            case libzerocoin::CoinDenomination::ZQ_TEN:
                ui->labelzDenom3Amount->setText (QString::number(spread.at(m)));
                break;
            case libzerocoin::CoinDenomination::ZQ_FIFTY:
                ui->labelzDenom4Amount->setText (QString::number(spread.at(m)));
                break;
            case libzerocoin::CoinDenomination::ZQ_ONE_HUNDRED:
                ui->labelzDenom5Amount->setText (QString::number(spread.at(m)));
                break;
            case libzerocoin::CoinDenomination::ZQ_FIVE_HUNDRED:
                ui->labelzDenom6Amount->setText (QString::number(spread.at(m)));
                break;
            case libzerocoin::CoinDenomination::ZQ_ONE_THOUSAND:
                ui->labelzDenom7Amount->setText (QString::number(spread.at(m)));
                break;
            case libzerocoin::CoinDenomination::ZQ_FIVE_THOUSAND:
                ui->labelzDenom8Amount->setText (QString::number(spread.at(m)));
                break;
            default:
                // Error Case: don't update display
                break;
        }
    }
    ui->labelzAvailableAmount->setText(QString::number(zerocoinBalance/COIN) + QString(" zPIV"));
    ui->labelzAvailableAmount_2->setText(QString::number(zerocoinBalance/COIN) + QString(" zPIV"));
    ui->labelzPIVAmountValue->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance - immatureBalance, false, BitcoinUnits::separatorAlways));
}

void PrivacyDialog::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if (currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentZerocoinBalance, 
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);
    }
}

void PrivacyDialog::showOutOfSyncWarning(bool fShow)
{
    ui->labelzPIVSyncStatus->setVisible(fShow);
}
