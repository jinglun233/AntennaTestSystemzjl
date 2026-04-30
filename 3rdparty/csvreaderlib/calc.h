#ifndef CALC_H
#define CALC_H

#include "CsvReaderLib_global.h"
#include <QList>
#include <QByteArray>
#include <QString>
#include <QVector>  // 新增：引入QVector头文件
#include "DataItem.h"

class CSVREADERLIB_EXPORT Calc
{
public:
    Calc(const QString &path);
    QByteArray Display2Bit(QByteArray data, int index, const QString &display);
    QString Bit2Display(const QByteArray &data, int index);

private:
    // ========== 修复1：QStringList → QVector<QString>（支持resize） ==========
    QVector<QString> m_Sdic;        // 替换QStringList，兼容resize
    QList<DataItem> m_ini;

    QList<DataItem> getIni(const QList<DataItem> &conf);
    QByteArray claString2Bit(const DataItem &temp, const QString &Display, QByteArray data);
    QByteArray ClaState2Bit(const DataItem &temp, const QString &Display, QByteArray data);
    QByteArray ClcAnalog2Bit(const DataItem &temp, const QString &Display, QByteArray data);
    QString FindBit(const QByteArray &byt, const DataItem &ini);
    QString ClaBit2string(const DataItem &tempIni, const QByteArray &tempData);
    QString ClaBit2State(const DataItem &temp, const QByteArray &byt);
    QString ClaBit2Analog(const DataItem &tempIni, const QByteArray &tempData);
};

#endif // CALC_H
