#include "comparereport.h"
#include "ui_comparereport.h"

#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QString>
#include <QDir>
#include <QMessageBox>

CompareReport::CompareReport(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CompareReport)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

CompareReport::~CompareReport()
{
    delete ui;
}

void CompareReport::addAccepted(const QString &s)
{
    ui->plainTextEditMatches->appendPlainText(s);
    ui->plainTextEditMatches->appendPlainText(QString());
}

void CompareReport::addMiss(const QString &s)
{
    ui->plainTextEditMisses->appendPlainText(s);
    ui->plainTextEditMisses->appendPlainText(QString());
}

void CompareReport::setCompareTables(const QString &source, const QString &check)
{
    ui->labelCheckTabel->setText(check);
    ui->labelSourceTable->setText(source);
}

void CompareReport::on_pushButtonSaveMatches_clicked()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Export matches..."), QDir::homePath());
    if (!fn.isEmpty())
    {
        QFile f(fn);
        QTextStream ts(&f);
        if (!f.open(QIODevice::Truncate | QIODevice::WriteOnly))
        {
            QMessageBox::warning(this, tr("Error"), QString(tr("Could not write to file.")));
            return;
        }
        ts << ui->plainTextEditMatches->toPlainText();
        ts.flush();
        f.close();
    }
}

void CompareReport::on_pushButtonSaveMisses_clicked()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Export misses..."), QDir::homePath());
    if (!fn.isEmpty())
    {
        QFile f(fn);
        QTextStream ts(&f);
        if (!f.open(QIODevice::Truncate | QIODevice::WriteOnly))
        {
            QMessageBox::warning(this, tr("Error"), QString(tr("Could not write to file.")));
            return;
        }
        ts << ui->plainTextEditMisses->toPlainText();
        ts.flush();
        f.close();
    }
}
