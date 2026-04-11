#ifndef RATESPROVIDER_H
#define RATESPROVIDER_H

#include "models.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QByteArray>
#include <QUrl>

class RatesProvider : public QObject
{
    Q_OBJECT

public:
    explicit RatesProvider(QObject *parent = nullptr);

    QVector<CurrencyRate> updateCurrencyRates(QString *message = nullptr);
    QVector<RateRecord> updateCreditRates(QString *message = nullptr);
    QVector<RateRecord> updateDepositRates(QString *message = nullptr);

    QVector<CurrencyRate> currencyRatesWithFallback(QString *message = nullptr);
    QVector<RateRecord> creditRatesWithFallback(QString *message = nullptr);
    QVector<RateRecord> depositRatesWithFallback(QString *message = nullptr);

private:
    QByteArray getUrl(const QUrl &url, QString *error = nullptr);
    QVector<CurrencyRate> parseCbrXml(const QByteArray &data) const;
    QVector<RateRecord> parseRatesPage(const QByteArray &data, const QString &source, bool deposits) const;
    QVector<RateRecord> demoCreditRates() const;
    QVector<RateRecord> demoDepositRates() const;
    QVector<CurrencyRate> demoCurrencyRates() const;

    QNetworkAccessManager m_manager;
};

#endif // RATESPROVIDER_H
