
#include "addresstablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "wallet.h"
#include "base58.h"
#include "stealth.h"

#include <QFont>
#include <QDebug>

const QString AddressTableModel::Send = "S";
const QString AddressTableModel::Receive = "R";

struct AddressTableEntry
{
    enum Type {
        Sending,
        Receiving
    };

    Type type;
    QString label;
    QString address;
    bool stealth;

    AddressTableEntry() {}