#include "guiutil.h"
#include "guiconstants.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "messagemodel.h"
#include "addresstablemodel.h"

#include "ui_interface.h"
#include "base58.h"
#include "json_spirit.h"

#include <QSet>
#include <QTimer>
#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include <QFont>
#include <QColor>

#include <boost/bind.hpp>

Q_DECLARE_METATYPE(std::vector<unsigned char>);

QList<QString> ambiguous; /**< Specifies Ambiguous addresses */

const QString MessageModel::Sent = "Sent";
const QString MessageModel::Received = "Received";

struct MessageTableEntryLessThan
{
    bool operator()(const MessageTableEntry &a, const MessageTableEntry &b) const {return a.received_datetime < b.received_datetime;};
    bool operator()(const MessageTableEntry &a, const QDateTime         &b) const {return a.received_datetime < b;}
    bool operator()(const QDateTime         &a, const MessageTableEntry &b) const {return a < b.received_datetime;}
};

// Private implementation
class MessageTablePriv
{
public:
    QList<MessageTableEntry> cachedMessageTable;
    MessageModel *parent;

    MessageTablePriv(MessageModel *parent):
        parent(parent) {}

    void refreshMessageTable()
    {
        cachedMessageTable.clear();
        
        if (parent->getWalletModel()->getEncryptionStatus() == WalletModel::Locked)
        {
            // -- messages are stored encrypted, can't load them without the private keys
            return;
        };

        {
            LOCK(cs_smsgDB);

            SecMsgDB dbSmsg;

            if (!dbSmsg.Open("cr+"))
                //throw runtime_error("Could not open DB.");
                return;

            unsigned char chKey[18];
            std::vector<unsigned char> vchKey;
            vchKey.resize(18);

            SecMsgStored smsgStored;
            MessageData msg;
            QString label;
            QDateTime sent_datetime;
            QDateTime received_datetime;

            std::string sPrefix("im");
            leveldb::Iterator* it = dbSmsg.pdb->NewIterator(leveldb::ReadOptions());
            while (dbSmsg.NextSmesg(it, sPrefix, chKey, smsgStored))
            {
                uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;
                if (SecureMsgDecrypt(false, smsgStored.sAddrTo, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
                {
                    label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(msg.sFromAddress));

                    sent_datetime    .setTime_t(msg.timestamp);
                    received_datetime.setTime_t(smsgStored.timeReceived);
                    
                    memcpy(&vchKey[0], chKey, 18);

                    addMessageEntry(MessageTableEntry(vchKey,
                                                      MessageTableEntry::Received,
                                                      label,
                                                      QString::fromStdString(smsgStored.sAddrTo),
                                                      QString::fromStdString(msg.sFromAddress),
                                                      sent_datetime,
                                                      received_datetime,
                                                      (char*)&msg.vchMessage[0]),
                                    true);
                }
            };

            delete it;

            sPrefix = "sm";
            it = dbSmsg.pdb->NewIterator(leveldb::ReadOptions());
            while (dbSmsg.NextSmesg(it, sPrefix, chKey, smsgStored))
            {
                uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;
                if (SecureMsgDecrypt(false, smsgStored.sAddrOutbox, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
                {
                    label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(smsgStored.sAddrTo));

                    sent_datetime    .setTime_t(msg.timestamp);
                    received_datetime.setTime_t(smsgStored.timeReceived);
                    
                    memcpy(&vchKey[0], chKey, 18);

                    addMessageEntry(MessageTableEntry(vchKey,
                                                      MessageTableEntry::Sent,
                                                      label,
                                                      QString::fromStdString(smsgStored.sAddrTo),
                                                      QString::fromStdString(msg.sFromAddress),
                                                      sent_datetime,
                                                      received_datetime,
                                                      (char*)&msg.vchMessage[0]),
                                    true);
                }
            };

            delete it;
        }
    }

    void newMessage(const SecMsgStored& inboxHdr)
    {
        // we have to copy it, because it doesn't like constants going into Decrypt
        SecMsgStored smsgStored = inboxHdr;
        MessageData msg;
        QString label;
        QDateTime sent_datetime;
        QDateTime received_datetime;

        uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;
        if (SecureMsgDecrypt(false, smsgStored.sAddrTo, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
        {
            label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(msg.sFromAddress));

            sent_datetime    .setTime_t(msg.timestamp);
            received_datetime.setTime_t(smsgStored.timeReceived);

            std::string sPrefix("im");
            SecureMessage* psmsg = (SecureMessage*) &smsgStored.vchMessage[0];
            
            std::vector<unsigned char> vchKey;
            vchKey.resize(18);
            memcpy(&vchKey[0],  sPrefix.data(),  2);
            memcpy(&vchKey[2],  &psmsg->timestamp, 8);
            memcpy(&vchKey[10], &smsgStored.vchMessage[SMSG_HDR_LEN], 8);    // sample

            addMessageEntry(MessageTableEntry(vchKey,
                                              MessageTableEntry::Received,
                                              label,
                                              QString::fromStdString(smsgStored.sAddrTo),
                                              QString::fromStdString(msg.sFromAddress),
                                              sent_datetime,
                                              received_datetime,
                                              (char*)&msg.vchMessage[0]),
                            false);
        }
    }

    void newOutboxMessage(const SecMsgStored& outboxHdr)
    {

        SecMsgStored smsgStored = outboxHdr;
        MessageData msg;
        QString label;
        QDateTime sent_datetime;
        QDateTime received_datetime;

        uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;
        if (SecureMsgDecrypt(false, smsgStored.sAddrOutbox, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
        {
            label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(smsgStored.sAddrTo));

            sent_datetime    .setTime_t(msg.timestamp);
            received_datetime.setTime_t(smsgStored.timeReceived);

            std::string sPrefix("sm");
            SecureMessage* psmsg = (SecureMessage*) &smsgStored.vchMessage[0];
            std::vector<unsigned char> vchKey;
            vchKey.resize(18);
            memcpy(&vchKey[0],  sPrefix.data(),  2);
            memcpy(&vchKey[2],  &psmsg->timestamp, 8);
            memcpy(&vchKey[10], &smsgStored.vchMessage[SMSG_HDR_LEN], 8);    // sample

            addMessageEntry(MessageTableEntry(vchKey,
                                              MessageTableEntry::Sent,
                                              label,
                                              QString::fromStdString(smsgStored.sAddrTo),
                                              QString::fromStdString(msg.sFromAddress),
                                              sent_datetime,
                                              received_datetime,
                                              (char*)&msg.vchMessage[0]),
                            false);
        }
    }

    void walletUnlocked()
    {
        // -- wallet is unlocked, can get at the private keys now
        refreshMessageTable();
        
        parent->reset(); // reload table view
        
        if (parent->proxyModel)
        {
            parent->proxyModel->setFilterRole(false);
            parent->proxyModel->setFilterFixedString("");
            parent->resetFilter();
            parent->proxyModel->setFilterRole(MessageModel::Ambiguous);
            parent->proxyModel->setFilterFixedString("true");
        }
        
        //invalidateFilter()
    }
    
    void setEncryptionStatus(int status)
    {
        if (status == WalletModel::Locked)
        {
            // -- Wallet is locked, clear secure message display.
            cachedMessageTable.clear();

            parent->reset(); // reload table view
        };
    };

    MessageTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedMessageTable.size())
       