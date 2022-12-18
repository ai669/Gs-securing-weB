#ifndef SIGNVERIFYMESSAGEDIALOG_H
#define SIGNVERIFYMESSAGEDIALOG_H

#include <QDialog>

namespace Ui {
    class SignVerifyMessageDialog;
}
class WalletModel;

class SignVerifyMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SignVerifyMessageDialog(QWidget *parent = 0);
    ~SignVerifyMessageDialog();

    void setModel(Walle