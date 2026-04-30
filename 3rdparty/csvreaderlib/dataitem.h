#ifndef DATAITEM_H
#define DATAITEM_H

#include "CsvReaderLib_global.h" // 引入导出宏
#include <QString>
#include <QVariantList>
#include <QMap>
#include <QByteArray>
#include <QPair>
#include <QException>

// 关键：添加CSVREADERLIB_EXPORT导出类
class CSVREADERLIB_EXPORT DataItem
{
public:
    explicit DataItem(const QVariantList &frameInfo);

    // 可读写属性
    int bLen1() const;
    void setBLen1(int bLen);

    QMap<QString, QString> displayDictionary1() const;
    void setDisplayDictionary1(const QMap<QString, QString> &dict);

    double d0() const;
    void setD0(double d0);

    double d1() const;
    void setD1(double d1);

    double df1() const;
    void setDf1(double df1);

    QMap<QString, QString> stateDictionary() const;
    void setStateDictionary(const QMap<QString, QString> &dict);

    // 只读属性
    int index() const;
    int startByte() const;
    int startBit() const;
    QString dataType() const;
    int dataLen() const;
    int bLen() const;
    QString dataName() const;
    QString dataScop() const;

private:
    QPair<QMap<QString, QString>, QMap<QString, QString>> splitData(const QString &dataScop);
    int lenTurnToBLen(int len);

    // 私有成员
    int m_index;
    int m_startByte;
    int m_startBit;
    QString m_dataType;
    int m_len;
    int m_bLen;
    QString m_dataName;
    QString m_dataScop;
    QMap<QString, QString> m_stateDictionary;
    QMap<QString, QString> m_displayDictionary;
    double m_d0;
    double m_d1;
    double m_df1;
};

#endif // DATAITEM_H
