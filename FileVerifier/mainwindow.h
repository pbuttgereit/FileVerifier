#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

    void on_actionFile_s_triggered();

    void on_actionDirectory_triggered();


    void on_actionDatabase_triggered();

    void on_comboBoxTables_activated(const QString &arg1);

    void on_actionFile_triggered();

    void on_actionAll_files_triggered();

    void on_actionCurrent_Table_triggered();

private:
    void m_updateEntryList(const QStringList &l);
    bool setupDatabase();
    void compareHashes(const QString &checkTable,const QString &sourceTable);
    Ui::MainWindow *ui;
    QStringList m_entryList;
    QProgressBar *m_PBar;
    QSqlDatabase db;
    QSqlTableModel *m_model;
};

#endif // MAINWINDOW_H
