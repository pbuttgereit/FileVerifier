#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QProgressBar>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QDateTime>
#include <QTableView>
#include <QSqlTableModel>
#include <QSqlError>
#include <QComboBox>
#include <QSqlRecord>
#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QDirIterator>
#include <QMessageBox>

#include <QDebug>

#include "comparereport.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), m_EntryList(), m_Database(), m_Model(nullptr)
{
    ui->setupUi(this);

    m_Database = QSqlDatabase::addDatabase("QSQLITE", "FileVerifier");

    m_Model = new QSqlTableModel(this, m_Database);
    ui->tableView->setModel(m_Model);
}

MainWindow::~MainWindow()
{
    if (m_Database.isValid())
    {
        m_Database.close();
    }

    delete ui;
}

void MainWindow::updateEntryList(const QStringList &l)
{
    if (l.length() == 0)
    {
        return;
    }

    // clear existing entry list
    m_EntryList.clear();

    ui->progressBar->setRange(1, l.length());
    ui->progressBar->setValue(0);

    // l is a list of one or more files or directories
    // recursively get all file names starting with this list
    for (int i = 0; i < l.length(); ++i)
    {
        QFileInfo fi(l.at(i));
        if (fi.isDir())
        {
            // recursively get all files in dir and append to m_entryList (skip sym-links and .*/ ..*)
            QDir dir(l.at(i));
            QDirIterator it(l.at(i), QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext())
            {
                m_EntryList << it.next();
            }
        }
        else //is a file
        {
            m_EntryList << l.at(i);
        }
        ui->progressBar->setValue(i);
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0, m_EntryList.length());

    QString parent;
    if (l.length() > 1)
    {
        QFileInfo fi(l.at(1));
        parent = fi.path();
    }
    else
    {
        QFileInfo fi(l.at(0));
        if (fi.isDir())
        {
            parent = l.at(0);
        }
        else
        {
            parent = fi.path();
        }
    }

    // Strip path
    QString parentPath = QFileInfo(parent).path();
    parent = QFileInfo(parent).fileName();

    // Transfer the data into our database
    // Create a new table with name consisting of parent path and timestamp
    QString tableName(QString("%1%2").arg("A").arg(QDateTime::currentSecsSinceEpoch()));
    QSqlQuery query(m_Database);

    if (!query.exec(QString("CREATE TABLE IF NOT EXISTS '%1' ( "
                            "id integer primary key autoincrement, "
                            "path text,"
                            "fileName text,"
                            "lastChanged text,"
                            "hashType text,"
                            "hash text,"
                            "hashDate text"
                            " )"
                            ).arg(tableName)
                    )
            )
    {
        QMessageBox::warning(this, tr("Error"), tr("Could not create table in database."));
        qDebug() << query.lastQuery();
        qDebug() << query.lastError().text();
        return;
    }

    // Loop over entries and put into database
    qDebug() << "Transaction: " << m_Database.transaction();

    QCryptographicHash hashme(QCryptographicHash::Sha512);
    for (int i = 0; i < m_EntryList.length(); ++i)
    {
        QFileInfo fi(m_EntryList.at(i));
        QFile *file = new QFile(m_EntryList.at(i));
        file->open(QIODevice::ReadOnly);
        hashme.reset();
        while (!file->atEnd())
        {
            hashme.addData(file->read(8192));
        }

        // Prepare injection-safe query
        query.prepare(QString("INSERT INTO '%1' (path, fileName, lastChanged, hashType, hash, hashDate) VALUES (?, ?, ?, ?, ?, ?)").arg(tableName));

        query.addBindValue(QString(fi.path()).remove(parentPath + "/"));
        query.addBindValue(fi.fileName());
        query.addBindValue(fi.lastModified().toString("yyyy-MM-dd HH:mm.ss"));
        query.addBindValue("SHA_512");
        query.addBindValue(QString::fromUtf8(hashme.result().toHex()));
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm.ss"));

        if (!query.exec())
        {
            QMessageBox::warning(this, tr("Error"), tr("Error inserting into table."));
            qDebug() << query.lastQuery();
            qDebug() << query.lastError().text();
            file->close();
            delete file;
            return;
        }

        ui->progressBar->setValue(i);
        file->close();
        delete file;
    }

    qDebug() << "Commit:" << m_Database.commit();
    ui->progressBar->setValue(0);

    ui->comboBoxTables->addItem(tableName);
    ui->comboBoxTables->setCurrentText(tableName);

    on_comboBoxTables_activated(tableName);
}

void MainWindow::compareHashes(const QString &checkTable, const QString &sourceTable)
{
    // loop over checktables entries and identify those in the source table, compare hashes
    QSqlQuery checkQuery(m_Database);

    int numFiles = 0;
    if (checkQuery.exec(QString("SELECT count(id) FROM '%1'").arg(checkTable)))
    {
        if (checkQuery.next())
        {
            numFiles = checkQuery.value(0).toInt();
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Error"), tr("Error getting number of files."));
        return;
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0, numFiles);
    int ctr = 0;

    CompareReport *report = new CompareReport(this);
    report->setCompareTables(sourceTable, checkTable);

    if (checkQuery.exec(QString("SELECT path,fileName,hash FROM '%1'").arg(checkTable)))
    {
        QSqlQuery sourceQuery(m_Database);
        while (checkQuery.next())
        {
            ui->progressBar->setValue(++ctr);

            if (sourceQuery.exec(QString("SELECT path,fileName FROM '%1' WHERE hash='%2'")
                                 .arg(sourceTable)
                                 .arg(checkQuery.value("hash").toString())
                                 )
                    )
            {
                if (sourceQuery.next()) // matching file found
                {
                    report->addAccepted(QString("%1\t%2\t%3\t%4\t%5")
                                        .arg(checkQuery.value("path").toString())
                                        .arg(checkQuery.value("fileName").toString())
                                        .arg(checkQuery.value("hash").toString())
                                        .arg(sourceQuery.value("fileName").toString())
                                        .arg(sourceQuery.value("path").toString()));
                }
                else // no match found
                {
                    report->addMiss(QString("%1\t%2\t%3\t-\t-")
                                    .arg(checkQuery.value("path").toString())
                                    .arg(checkQuery.value("fileName").toString())
                                    .arg(checkQuery.value("hash").toString()));
                }
            }
        }

        ui->progressBar->setValue(0);
        report->show();
    }
    else
    {
        qDebug() << checkQuery.lastQuery();
        qDebug() << checkQuery.lastError().text();
    }
}

void MainWindow::on_comboBoxTables_activated(const QString &table)
{
    m_Model->setTable(table);
    m_Model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_Model->select();

    ui->tableView->hideColumn(0); // Hide internal id in table view
}

void MainWindow::on_actionCurrent_Table_triggered()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save export..."), "Export.txt", tr("Table Exports (*.txt)"));
    if (fn.isEmpty())
    {
        return;
    }

    QFile f(fn);
    QTextStream ts(&f);
    if (!f.open(QIODevice::ReadWrite))
    {
        QMessageBox::warning(this, tr("Error"), QString(tr("Could not write to file.")));
        return;
    }

    QString tableName = m_Model->tableName();
    QSqlQuery query(m_Database);
    ts << "path\tfileName\tlastChanged\thashType\thash\thashDate\t\r\n";
    query.exec(QString("SELECT path,fileName,lastChanged,hashType,hash,hashDate FROM %1").arg(tableName));
    while (query.next())
    {
        for (int i = 0; i < 6; ++i)
        {
            ts << query.value(i).toString() << "\t";
        }
        ts << "\r\n";
    }

    ts.flush();
    f.close();
}

void MainWindow::updateDisplay()
{
    // Select existing tables and fill combo box
    QSqlQuery query(m_Database);

    if (query.exec("SELECT name FROM sqlite_master WHERE type ='table' AND name NOT LIKE 'sqlite_%';"))
    {
        // Remove previous selections
        ui->comboBoxTables->clear();
        m_Model->clear();

        while (query.next())
        {
            ui->comboBoxTables->addItem((query.value(0).toByteArray()));
        }

        if (ui->comboBoxTables->count() > 0)
        {
            m_Model->setTable(ui->comboBoxTables->itemText(0));
            m_Model->setEditStrategy(QSqlTableModel::OnManualSubmit);
            m_Model->select();

            ui->tableView->hideColumn(0); // Hide internal id in table view
        }
    }
}

void MainWindow::on_actionDbNew_triggered()
{
    QString dbName = QFileDialog::getSaveFileName(this, "Create new database...", QDir::homePath(), tr("FileVerifier Database (*.fvdb)"), nullptr);
    if(!dbName.isEmpty())
    {
        // Close open database
        m_Database.close();

        // Create new database file
        m_Database.setDatabaseName(dbName);

        if (!m_Database.open())
        {
            QMessageBox::warning(this, tr("Error"), QString(tr("Database could not be created.")));
        }
        else
        {
            // Select existing tables
            QSqlQuery query(m_Database);
            QStringList tableNames;

            if (query.exec("SELECT name FROM sqlite_master WHERE type ='table' AND name NOT LIKE 'sqlite_%';"))
            {
                while (query.next())
                {
                    tableNames.append(query.value(0).toByteArray());
                }
            }

            // Li'l bobby droppin' tables we call him
            for (int i = 0; i < tableNames.count(); i++)
            {
                query.exec(QString("DROP TABLE '%1';").arg(tableNames.at(i)));
            }

            updateDisplay();
        }
    }
}

void MainWindow::on_actionDbOpen_triggered()
{
    QString dbName = QFileDialog::getOpenFileName(this, "Select database...", QDir::homePath(), tr("FileVerifier Database (*.fvdb)"));

    if(!dbName.isEmpty())
    {
        m_Database.close();
        m_Database.setDatabaseName(dbName);

        if (!m_Database.open())
        {
            QMessageBox::warning(this, tr("Error"), tr("Could not open database."));
        }
        else
        {
            updateDisplay();
        }
    }
}

bool MainWindow::hashFiles()
{
    QStringList l = QFileDialog::getOpenFileNames(this, tr("Select files to be hashed..."));
    if (l.length() > 0)
    {
        // remember directory we opened
        QFileInfo fi(l.at(0));
        QString p = fi.filePath();

        // update entryList
        updateEntryList(l);

        return true;
    }

    return false; // No files were selected
}

bool MainWindow::hashDirectory()
{
    QString l = QFileDialog::getExistingDirectory(this, tr("Select directory for recursive file hashing..."));
    if (!l.isEmpty())
    {
        // update entryList
        updateEntryList(QStringList() << l);

        return true;
    }

    return false; // No directory was selected
}

void MainWindow::on_actionHashFile_triggered()
{
    hashFiles();
}

void MainWindow::on_actionHashDirectory_triggered()
{
    hashDirectory();
}

void MainWindow::on_actionVerifyFile_triggered()
{
    QString sourceTable = m_Model->tableName();
    if(hashFiles())
    {
        QString checkTable = m_Model->tableName();
        compareHashes(checkTable, sourceTable);
    }
}

void MainWindow::on_actionVerifyDirectory_triggered()
{
    QString sourceTable = m_Model->tableName();
    if(hashDirectory())
    {
        QString checkTable = m_Model->tableName();
        compareHashes(checkTable, sourceTable);
    }
}

void MainWindow::on_actionE_xit_triggered()
{
    close();
}

void MainWindow::on_actionAbout_triggered()
{
    /* Display about */
    QMessageBox::about(this, tr("About"), QString("<table cellspacing=0 cellpadding=0><tr><td valign=\"top\"><img src=\":/FileVerifier.png\" alt=\"\" />"
                                                  "</td><td><p>&nbsp;&nbsp;&nbsp;</p></td><td valign=\"top\"><p><b><font size=\"+2\">FileVerifier ") + QString(APP_VERSION) + QString("</font></b>") +
                       QString("<br /><br />Recursively create and check file hashes.<br />Software is offered without warranty.</p></td></tr></table>") +
                       QString("<p>Copyright &copy; 2019-2020 P. Buttgereit, B. Martensen<br /><a href=\"https://github.com/bmartensen/FileVerifier\">https://github.com/bmartensen/FileVerifier</a></p>"));


}

void MainWindow::on_actionAbout_Qt_triggered()
{
    /* Thanks to Qt. */
    QMessageBox::aboutQt(this, "Qt");
}
