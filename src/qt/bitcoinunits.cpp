// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinunits.h"
#include "main.h"

#include <QSettings>
#include <QStringList>

WayaWolfCoinUnits::WayaWolfCoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<WayaWolfCoinUnits::Unit> WayaWolfCoinUnits::availableUnits()
{
    QList<WayaWolfCoinUnits::Unit> unitlist;
    unitlist.append(WW);
    unitlist.append(mWW);
    unitlist.append(uWW);
    return unitlist;
}

bool WayaWolfCoinUnits::valid(int unit)
{
    switch(unit)
    {
    case WW:
    case mWW:
    case uWW:
        return true;
    default:
        return false;
    }
}

QString WayaWolfCoinUnits::name(int unit)
{
    switch(unit)
    {
    case WW: return QString("WW");
    case mWW: return QString("mWW");
    case uWW: return QString::fromUtf8("μWW");
    default: return QString("???");
    }
}

QString WayaWolfCoinUnits::description(int unit)
{
    switch(unit)
    {
    case WW: return QString("WayaWolfCoins");
    case mWW: return QString("Milli-WayaWolfCoins (1 / 1,000)");
    case uWW: return QString("Micro-WayaWolfCoins (1 / 1,000,000)");
    default: return QString("???");
    }
}

qint64 WayaWolfCoinUnits::factor(int unit)
{
    switch(unit)
    {
    case WW:  return 100000000;
    case mWW: return 100000;
    case uWW: return 100;
    default:   return 100000000;
    }
}

int WayaWolfCoinUnits::amountDigits(int unit)
{
    switch(unit)
    {
    case WW: return 11;  // 21,000,000,000         (# digits, without commas)
    case mWW: return 14; // 21,000,000,000,000
    case uWW: return 17; // 21,000,000,000,000,000
    default: return 0;
    }
}

int WayaWolfCoinUnits::decimals(int unit)
{
    switch(unit)
    {
    case WW: return 8;
    case mWW: return 5;
    case uWW: return 2;
    default: return 0;
    }
}

QString WayaWolfCoinUnits::format(int unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 n = (qint64)nIn;
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    // Right-trim excess zeros after the decimal point
    int nTrim = 0;
    for (int i = remainder_str.size()-1; i>=2 && (remainder_str.at(i) == '0'); --i)
        ++nTrim;
    remainder_str.chop(nTrim);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');
    return quotient_str + QString(".") + remainder_str;
}

// TODO: WayaWolfCoiniew all remaining calls to WayaWolfCoinUnits::formatWithUnit to
// TODO: determine whether the output is used in a plain text context
// TODO: or an HTML context (and replace with
// TODO: BtcoinUnits::formatHtmlWithUnit in the latter case). Hopefully
// TODO: there aren't instances where the result could be used in
// TODO: either context.

// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.

QString WayaWolfCoinUnits::formatWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    return format(unit, amount, plussign, separators) + QString(" ") + name(unit);
}

QString WayaWolfCoinUnits::formatHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle se