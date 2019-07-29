#ifndef COMPAREREPORT_H
#define COMPAREREPORT_H

#include <QDialog>

namespace Ui {
class CompareReport;
}

class CompareReport : public QDialog
{
    Q_OBJECT

public:
    explicit CompareReport(QWidget *parent = nullptr);
    ~CompareReport();
    void addAccepted(const QString &s);
    void addMiss(const QString &s);
    void setCompareTables(const QString &source, const QString &check);

private slots:

    void on_pushButtonSaveMatches_clicked();

    void on_pushButtonSaveMisses_clicked();

private:
    Ui::CompareReport *ui;
};

#endif // COMPAREREPORT_H
