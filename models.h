#ifndef MODELS_H
#define MODELS_H

#include <QDateTime>
#include <QString>

struct FinanceOperation
{
    enum Type {
        Income = 0,
        Expense = 1
    };

    int id = -1;
    int userId = -1;
    Type type = Expense;
    QString category;
    double amount = 0.0;
    QDateTime date;
    QString note;
};

struct RateRecord
{
    int id = -1;
    QString bank;
    double rate = 0.0;
    QString description;
    QString source;
    QString fetchedAt;
};

struct CurrencyRate
{
    int id = -1;
    QString code;
    QString name;
    double value = 0.0;
    QString source;
    QString fetchedAt;
};

#endif // MODELS_H
