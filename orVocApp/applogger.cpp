// orVocApp/applogger.cpp
#include "applogger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

QString AppLogger::filePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + "/app.log";
}

void AppLogger::clear()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);

    QFile file(filePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.close();
}

void AppLogger::log(const QString &message)
{
    QFile file(filePath());
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString(Qt::ISODate) << " " << message << "\n";
    file.close();
}
