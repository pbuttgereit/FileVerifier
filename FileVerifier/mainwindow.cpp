#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QStatusBar>
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

#include <QDebug>

#include "comparereport.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),m_entryList(),m_PBar(nullptr),db(),m_model(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("");
    m_PBar=new QProgressBar();
    statusBar()->addPermanentWidget(m_PBar);

    db=QSqlDatabase::addDatabase("QSQLITE","FileVerifier");
    m_model=new QSqlTableModel(this,db);
    ui->tableView->setModel(m_model);
}

MainWindow::~MainWindow()
{
    if(db.isValid())
    {
        db.close();
    };

    delete ui;
}

/**
 * @brief MainWindow::on_actionOpen_triggered
 * open file dialog to select one or more existing files
 */
void MainWindow::on_actionFile_s_triggered()
{
    QSettings s;
    QString dir=s.value("Directory",QDir::homePath()).toString();
    QStringList l=QFileDialog::getOpenFileNames(this,tr("Select files to be hashed"),dir);
    if(l.length()>0)
    {
        //remember directory we opened
        QFileInfo fi(l.at(0));
        QString p=fi.filePath();
        s.setValue("Directory",p);

        //update entryList
        m_updateEntryList(l);
    }
}

/**
 * @brief MainWindow::on_actionDirectory_triggered
 * open file dialog to select one existing directory
 * */
void MainWindow::on_actionDirectory_triggered()
{
    QSettings s;
    QString dir=s.value("Directory",QDir::homePath()).toString();
    QString l=QFileDialog::getExistingDirectory(this,tr("Select directory for recursive file hashing"),dir);

    if(!l.isEmpty())
    {
        //remember directory we opened
        s.setValue("Directory",dir);

        //update entryList
        m_updateEntryList(QStringList() << l);
    }
}

/**
 * @brief MainWindow::m_updateEntryList
 * @param l
 * update/fill the entry list of files that need to be hashed
 */
void MainWindow::m_updateEntryList(const QStringList &l)
{
    if(l.length()==0)
    {
        statusBar()->showMessage(tr("no files selected"));
        return;
    }

    //clear existing entry list
    m_entryList.clear();

    m_PBar->setRange(1,l.length());
    m_PBar->setValue(0);
    statusBar()->messageChanged(tr("creating file list"));

    //l is a list of one or more files or directories
    //recursively get all file names starting with this list
    for(int i=0;i<l.length();++i)
    {
        QFileInfo fi(l.at(i));
        if(fi.isDir())
        {
            //recursively get all files in dir and append to m_entryList (skip sym-links and .*/ ..*)
            QDir dir(l.at(i));
            //m_entryList << dir.entryList(QDir::Files|QDir::NoSymLinks|QDir::NoDotAndDotDot);
            QDirIterator it(l.at(i),QDir::Files|QDir::NoSymLinks|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
            while(it.hasNext())
            {
                m_entryList << it.next();
            }
        }
        else //is a file
        {
            m_entryList << l.at(i);
        }
        m_PBar->setValue(i);
    }
    m_PBar->setValue(0);
    m_PBar->setRange(0,m_entryList.length());
    statusBar()->messageChanged(tr("setup table for file list"));

    //get parent dir
    //selected files / dirs are on one level (inside one dir)
    //if only one dir is selected we take that as parent
    //else we take the parent dir
    QString parent;
    if(l.length()>1)
    {
        QFileInfo fi(l.at(1));
        parent=fi.path();
    }
    else
    {
        QFileInfo fi(l.at(0));
        if(fi.isDir())
        {
            parent=l.at(0);
        }
        else
        {
            parent=fi.path();
        }
    }

    //strip path
    QString parentPath=QFileInfo(parent).path();
    parent=QFileInfo(parent).fileName();

    //transfer the data into our database
    if(setupDatabase())
    {
        statusBar()->messageChanged(tr("creating database table"));

        //create a new table with name consisting of parent path and timestamp
        QString tableName(QString("%1%2").arg("A").arg(QDateTime::currentSecsSinceEpoch()));
        //convert to base64 to handle special characters
        QSqlQuery query(db);
        if(!query.exec(QString("create table if not exists '%1' ( "
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
            QMessageBox::warning(this,tr("DB ERROR"),tr("Could not create table in database"));
            qDebug() << query.lastQuery();
            qDebug() << query.lastError().text();
            return;
        }

        //loop over entries and put into database
        //qDebug() << "entryList: " << m_entryList;
        qDebug() << "transaction: " << db.transaction();

        QCryptographicHash hashme(QCryptographicHash::Sha512);
        for(int i=0;i<m_entryList.length();++i)
        {
            QFileInfo fi(m_entryList.at(i));
            QFile *file=new QFile(m_entryList.at(i));
            file->open(QIODevice::ReadOnly);
            hashme.reset();
            while(!file->atEnd())
            {
                hashme.addData(file->read(8192));
            }
            if(!query.exec(QString("insert into '%1' (path,fileName,lastChanged,hashType,hash,hashDate) values ('%2','%3','%4','%5','%6','%7')")
                                                .arg(tableName)
                                                .arg(QString(fi.path()).remove(parentPath+"/")) //keep only relative path
                                                .arg(fi.fileName())
                                                .arg(fi.lastModified().toString("yyyy-MM-dd HH:mm.ss"))
                                                .arg("SHA_512")
                                                .arg(QString::fromUtf8(hashme.result().toHex()))
                                                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm.ss"))
                           )
                    )
            {
                QMessageBox::warning(this,tr("DB ERROR"),tr("Error inserting into table"));
                qDebug() << query.lastQuery();
                qDebug() << query.lastError().text();
                file->close();
                delete file;
                return;
            }
            //qDebug() << fi.filePath();
            //qDebug() << parentPath;
            m_PBar->setValue(i);
            file->close();
            delete file;
        }
        qDebug() << "commit:" << db.commit();
        m_PBar->setValue(m_entryList.length());

        ui->comboBoxTables->addItem(tableName);
        ui->comboBoxTables->setCurrentText(tableName);
        on_comboBoxTables_activated(tableName);
    }
}

/**
 * @brief MainWindow::setupDatabase
 * @param db
 * @return
 * setup or open a sqlite database file
 */
bool MainWindow::setupDatabase()
{
    QSqlDatabase db=QSqlDatabase::database("FileVerifier");
    if(!db.isOpen())
    {
        //open database file or create new one
        on_actionDatabase_triggered();
    }

    if(!db.isOpen())
    {
        QMessageBox::warning(this,tr("DB ERROR"),QString(tr("Could not open database %1.")).arg(db.databaseName()));
        return false;
    }
    return true;
}

void MainWindow::compareHashes(const QString &checkTable, const QString &sourceTable)
{
    //loop over checktables entries and identify those in the source table, compare hashes
    QSqlQuery checkQuery(db);
    int numFiles=0;
    if(checkQuery.exec(QString("select count(id) from '%1'").arg(checkTable)))
    {
        if(checkQuery.next())
        {
            numFiles=checkQuery.value(0).toInt();
        }
    }
    else
    {
        QMessageBox::warning(this,tr("SQL Error"),tr("Error getting number of files"));
        return;
    }

    statusBar()->showMessage(tr("starting compare"));
    m_PBar->setValue(0);
    m_PBar->setRange(0,numFiles);
    int ctr=0;

    CompareReport *report=new CompareReport(this);
    report->setCompareTables(sourceTable,checkTable);

    if(checkQuery.exec(QString("select path,fileName,hash from '%1'").arg(checkTable)))
    {
        QSqlQuery sourceQuery(db);
        while(checkQuery.next())
        {
            m_PBar->setValue(++ctr);

            if(sourceQuery.exec(QString("select path,fileName from '%1' where hash='%2'")
                                                                .arg(sourceTable)
                                                                .arg(checkQuery.value("hash").toString())
                                )
                    )
            {
                if(sourceQuery.next()) //matching file found
                {
                    report->addAccepted(QString("%1\t%2\t%3\t%4\t%5")
                                .arg(checkQuery.value("path").toString())
                                .arg(checkQuery.value("fileName").toString())
                                .arg(checkQuery.value("hash").toString())
                                .arg(sourceQuery.value("fileName").toString())
                                .arg(sourceQuery.value("path").toString()));
                }
                else //no match found
                {
                    report->addMiss(QString("%1\t%2\t%3\t-\t-")
                                .arg(checkQuery.value("path").toString())
                                .arg(checkQuery.value("fileName").toString())
                                .arg(checkQuery.value("hash").toString()));
                }
            }
        }

        report->show();
    }
    else
    {
        qDebug() << checkQuery.lastQuery();
        qDebug() << checkQuery.lastError().text();
    }
}


/**
 * @brief MainWindow::on_actionDatabase_triggered
 * we store file lists with hashes in a sqlite database
 * for this, a database needs to be open / selected
 */
void MainWindow::on_actionDatabase_triggered()
{
    QSettings s;
    QString dbName=s.value("Database",QString("%1/FileVerifier.fvdb").arg(QDir::homePath())).toString();
    dbName=QFileDialog::getSaveFileName(this,"Select or create Database",dbName,tr("FileVerifier Database (*.fvdb)",nullptr,QFileDialog::DontConfirmOverwrite));
    if(db.databaseName()!=dbName)
    {
        db.setDatabaseName(dbName);
    }

    if(!db.open())
    {
        qDebug() << db.lastError().text();
        QMessageBox::warning(this,tr("DB ERROR"),QString(tr("Could not open %1")).arg(dbName));
        return;
    }
    s.setValue("Database",dbName);

    //select existing tables, offer choice (combo-box)
    QSqlQuery query(db);
    bool flag=false;
    if(query.exec("SELECT name FROM sqlite_master WHERE type ='table' AND name NOT LIKE 'sqlite_%';"))
    {
        while(query.next())
        {
            flag=true;
            ui->comboBoxTables->addItem(QString::fromUtf8(QByteArray::fromHex((query.value(0).toByteArray()))));
        }
    }
    else
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError().text();
    }
    if(!flag)statusBar()->messageChanged("loaded empty database");
}

void MainWindow::on_comboBoxTables_activated(const QString &table)
{
    m_model->setTable(table);
    //qDebug() << "set table: " << table.toUtf8().toHex();
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    ui->tableView->hideColumn(0);//don't show id
    m_model->select();
}

/*verify file hashes
 * use case 1: select a file and compare hash to previous one in db
 * use case 2: check all files again, assuming same directory layout
 *
 * in both cases, files are compared by hash
 *
 * procedure: start regular hashing in new db table
 * remember the current db table when started
 * loop over hashes and find matching hashes; output a list of filenames and hashes
 * /

/**
 * @brief MainWindow::on_actionFile_triggered
 * select one or more files to verify
 */
void MainWindow::on_actionFile_triggered()
{
    QString sourceTable=m_model->tableName();
    on_actionFile_s_triggered();
    QString checkTable=m_model->tableName();

    compareHashes(checkTable,sourceTable);
}

/**
 * @brief MainWindow::on_actionAll_files_triggered
 * check all files in current db table; give top directory to start
 */
void MainWindow::on_actionAll_files_triggered()
{
    QString sourceTable=m_model->tableName();
    on_actionDirectory_triggered();
    QString checkTable=m_model->tableName();

    compareHashes(checkTable,sourceTable);
}

void MainWindow::on_actionCurrent_Table_triggered()
{
    QString fn=QFileDialog::getSaveFileName(this,tr("Save Export"),QDir::homePath());
    if(fn.isEmpty())return;
    QFile f(fn);
    QTextStream ts(&f);
    if(!f.open(QIODevice::ReadWrite))
    {
        QMessageBox::warning(this,tr("IO ERROR"),QString(tr("Could not write to file %1")).arg(fn));
        return;
    }
    QString tableName=m_model->tableName();
    QSqlQuery query(db);
    ts << "path\tfileName\tlastChanged\thashType\thash\thashDate\t\n";
    query.exec(QString("select path,fileName,lastChanged,hashType,hash,hashDate from %1").arg(tableName));
    while(query.next())
    {
        for(int i=0;i<6;++i)
        {
            ts << query.value(i).toString() << "\t";
        }
        ts << "\n";
    }
    ts.flush();
    f.close();
}
