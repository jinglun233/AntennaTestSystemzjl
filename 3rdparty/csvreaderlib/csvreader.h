#ifndef CSVREADER_H
#define CSVREADER_H

#include "CsvReaderLib_global.h" // 引入导出宏
#include "dataitem.h"
#include <QList>
#include <QString>

// 关键：添加CSVREADERLIB_EXPORT导出类
class CSVREADERLIB_EXPORT CsvReader
{
public:
    static QList<DataItem> readCsvFile(const QString &csvPath, QString &errorMsg);

private:

    static QStringList parseCsvLines(const QString &fullContent);

    // 原有方法：保留
    static QStringList splitCsvLine(const QString &line);
};

#endif // CSVREADER_H
