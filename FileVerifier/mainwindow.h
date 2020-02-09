#pragma once

#include <QMainWindow>
#include <QStringList>
#include <QString>
#include <QSqlDatabase>

namespace Ui {
class MainWindow;
}

class QProgressBar;
class QSqlTableModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_comboBoxTables_activated(const QString &arg1);

    void on_actionCurrent_Table_triggered();

    void on_actionDbNew_triggered();

    void on_actionDbOpen_triggered();

    void on_actionHashFile_triggered();

    void on_actionHashDirectory_triggered();

    void on_actionVerifyFile_triggered();

    void on_actionVerifyDirectory_triggered();

    void on_actionE_xit_triggered();

    void on_actionAbout_triggered();

    void on_actionAbout_Qt_triggered();

private:
    void updateEntryList(const QStringList &l);
    void updateDisplay();

    bool hashFiles();
    bool hashDirectory();

    void compareHashes(const QString &checkTable, const QString &sourceTable);

    Ui::MainWindow *ui;

    QStringList m_EntryList;
    QSqlDatabase m_Database;

    QSqlTableModel *m_Model;
};
