#include "ratesprovider.h"
#include "database.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QList>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStringList>
#include <QSslSocket>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QtGlobal>
#include <QXmlStreamReader>

namespace {

constexpr int kMinimumRatesToShow = 3;

struct BankAlias
{
    QString alias;
    QString displayName;
};

const QVector<BankAlias> &bankAliases()
{
    static const QVector<BankAlias> aliases = {
        { QStringLiteral("Сбербанк"), QStringLiteral("Сбер") },
        { QStringLiteral("СберБанк"), QStringLiteral("Сбер") },
        { QStringLiteral("Сбер"), QStringLiteral("Сбер") },
        { QStringLiteral("Банк ВТБ"), QStringLiteral("ВТБ") },
        { QStringLiteral("ВТБ"), QStringLiteral("ВТБ") },
        { QStringLiteral("Альфа-Банк"), QStringLiteral("Альфа-Банк") },
        { QStringLiteral("Альфа"), QStringLiteral("Альфа-Банк") },
        { QStringLiteral("Газпромбанк"), QStringLiteral("Газпромбанк") },
        { QStringLiteral("Т-Банк"), QStringLiteral("Т-Банк") },
        { QStringLiteral("Тинькофф"), QStringLiteral("Т-Банк") },
        { QStringLiteral("Совкомбанк"), QStringLiteral("Совкомбанк") },
        { QStringLiteral("Промсвязьбанк"), QStringLiteral("ПСБ") },
        { QStringLiteral("ПСБ"), QStringLiteral("ПСБ") },
        { QStringLiteral("Россельхозбанк"), QStringLiteral("Россельхозбанк") },
        { QStringLiteral("МТС Банк"), QStringLiteral("МТС Банк") },
        { QStringLiteral("Открытие"), QStringLiteral("Открытие") },
        { QStringLiteral("Почта Банк"), QStringLiteral("Почта Банк") },
        { QStringLiteral("Уралсиб"), QStringLiteral("Уралсиб") },
        { QStringLiteral("Росбанк"), QStringLiteral("Росбанк") },
        { QStringLiteral("МКБ"), QStringLiteral("МКБ") },
        { QStringLiteral("Московский кредитный банк"), QStringLiteral("МКБ") },
        { QStringLiteral("Ренессанс"), QStringLiteral("Ренессанс Банк") },
        { QStringLiteral("Хоум Банк"), QStringLiteral("Хоум Банк") },
        { QStringLiteral("ОТП Банк"), QStringLiteral("ОТП Банк") },
        { QStringLiteral("Банк Синара"), QStringLiteral("Банк Синара") },
        { QStringLiteral("Русский Стандарт"), QStringLiteral("Русский Стандарт") },
        { QStringLiteral("Азиатско-Тихоокеанский банк"), QStringLiteral("АТБ") },
        { QStringLiteral("АТБ"), QStringLiteral("АТБ") },
        { QStringLiteral("Металлинвестбанк"), QStringLiteral("Металлинвестбанк") },
        { QStringLiteral("Банк «Пойдём!»"), QStringLiteral("Банк Пойдём") },
        { QStringLiteral("Пойдём"), QStringLiteral("Банк Пойдём") },
        { QStringLiteral("Банк ЗЕНИТ"), QStringLiteral("Банк ЗЕНИТ") },
        { QStringLiteral("Норвик Банк"), QStringLiteral("Норвик Банк") },
        { QStringLiteral("Credit.Club"), QStringLiteral("Credit.Club") },
        { QStringLiteral("Банк Жилищного Финансирования"), QStringLiteral("БЖФ Банк") },
        { QStringLiteral("Локо-Банк"), QStringLiteral("Локо-Банк") },
        { QStringLiteral("Свой Банк"), QStringLiteral("Свой Банк") },
        { QStringLiteral("БыстроБанк"), QStringLiteral("БыстроБанк") },
        { QStringLiteral("НС Банк"), QStringLiteral("НС Банк") },
        { QStringLiteral("МОРСКОЙ БАНК"), QStringLiteral("Морской Банк") },
        { QStringLiteral("Реалист"), QStringLiteral("Банк Реалист") },
        { QStringLiteral("КАМКОМБАНК"), QStringLiteral("Камкомбанк") },
        { QStringLiteral("УБРиР"), QStringLiteral("УБРиР") },
        { QStringLiteral("Экспобанк"), QStringLiteral("Экспобанк") },
        { QStringLiteral("Кубань Кредит"), QStringLiteral("Кубань Кредит") },
        { QStringLiteral("ДОМ.РФ"), QStringLiteral("Банк ДОМ.РФ") },
        { QStringLiteral("Абсолют Банк"), QStringLiteral("Абсолют Банк") },
        { QStringLiteral("Ак Барс"), QStringLiteral("Ак Барс Банк") },
        { QStringLiteral("Банк Санкт-Петербург"), QStringLiteral("Банк Санкт-Петербург") }
    };
    return aliases;
}

bool isPlaceholderBank(const QString &bank)
{
    const QString normalized = bank.trimmed().toCaseFolded();
    return normalized.isEmpty()
        || normalized == QStringLiteral("банк по кредиту")
        || normalized == QStringLiteral("банк по вкладу");
}

bool hasParserArtifact(const QString &text)
{
    const QString normalized = text.toCaseFolded();
    return normalized.contains(QStringLiteral("submitbutton"))
        || normalized.contains(QStringLiteral("tracking-url"))
        || normalized.contains(QStringLiteral("\"url\""))
        || normalized.contains(QStringLiteral("{\""))
        || normalized.contains(QStringLiteral("</"))
        || normalized.contains(QStringLiteral("<p"))
        || normalized.contains(QStringLiteral("class="))
        || normalized.contains(QStringLiteral("bestoffers"))
        || normalized.contains(QStringLiteral("styled__"))
        || normalized.contains(QStringLiteral("headcell"))
        || normalized.contains(QStringLiteral("href="))
        || normalized.contains(QStringLiteral("data-"))
        || normalized.contains(QStringLiteral(">"))
        || normalized.contains(QStringLiteral("<"))
        || normalized.contains(QStringLiteral("антимонополь"))
        || normalized.contains(QStringLiteral("признать незаконным"))
        || normalized.contains(QStringLiteral("банк россии"))
        || normalized.contains(QStringLiteral("системно знач"));
}

bool isPlausibleRate(double rate, bool deposits)
{
    if (deposits) {
        return rate >= 1.0 && rate <= 30.0;
    }
    return rate >= 3.0 && rate <= 45.0;
}

bool looksLikeProductDescription(const QString &text, bool deposits)
{
    const QString normalized = text.toCaseFolded();
    if (deposits) {
        return normalized.contains(QStringLiteral("вклад"))
            || normalized.contains(QStringLiteral("счет"))
            || normalized.contains(QStringLiteral("счёт"))
            || normalized.contains(QStringLiteral("накоп"))
            || normalized.contains(QStringLiteral("доход"))
            || normalized.contains(QStringLiteral("ставк"));
    }
    return normalized.contains(QStringLiteral("кредит"))
        || normalized.contains(QStringLiteral("налич"))
        || normalized.contains(QStringLiteral("заем"))
        || normalized.contains(QStringLiteral("займ"))
        || normalized.contains(QStringLiteral("ставк"));
}

QVector<RateRecord> usableRates(const QVector<RateRecord> &rates, bool deposits)
{
    QVector<RateRecord> result;
    for (const RateRecord &rate : rates) {
        if (isPlausibleRate(rate.rate, deposits)
            && !isPlaceholderBank(rate.bank)
            && !hasParserArtifact(rate.description)
            && looksLikeProductDescription(rate.description, deposits)) {
            result.append(rate);
        }
    }
    return result;
}

bool hasEnoughRates(const QVector<RateRecord> &rates)
{
    QStringList banks;
    for (const RateRecord &rate : rates) {
        const QString bank = rate.bank.trimmed().toCaseFolded();
        if (!bank.isEmpty() && !banks.contains(bank)) {
            banks.append(bank);
        }
    }
    return banks.size() >= kMinimumRatesToShow;
}

QString cleanedDescription(QString text)
{
    text.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    text.replace(QRegularExpression(QStringLiteral("[\"{}\\[\\]]")), QStringLiteral(" "));
    text.replace(QChar(0xFFFD), QStringLiteral(" "));
    text.replace(QStringLiteral("Банки.ру"), QStringLiteral(""));
    text.replace(QStringLiteral("Banki.ru"), QStringLiteral(""));
    text.replace(QRegularExpression(QStringLiteral("\\s*[•·]\\s*$")), QStringLiteral(""));
    text.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return text.trimmed().left(180);
}

QString normalizedLine(QString text)
{
    text.replace(QChar(0x00A0), QLatin1Char(' '));
    text.remove(QChar(0x200B));
    text = QTextDocumentFragment::fromHtml(text).toPlainText();
    text.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    text = text.trimmed();
    if (text.startsWith(QStringLiteral("Image:"), Qt::CaseInsensitive)) {
        text = text.mid(QStringLiteral("Image:").size()).trimmed();
    }
    if (text.startsWith(QStringLiteral("Изображение:"), Qt::CaseInsensitive)) {
        text = text.mid(QStringLiteral("Изображение:").size()).trimmed();
    }
    return text;
}

bool isServiceLine(const QString &line)
{
    const QString value = line.trimmed().toCaseFolded();
    return value.isEmpty()
        || value == QStringLiteral("подробнее")
        || value == QStringLiteral("далее")
        || value == QStringLiteral("сумма")
        || value == QStringLiteral("срок")
        || value == QStringLiteral("пск")
        || value == QStringLiteral("ставка")
        || value == QStringLiteral("фильтры")
        || value == QStringLiteral("по популярности")
        || value == QStringLiteral("все предложения")
        || value == QStringLiteral("показать еще")
        || value == QStringLiteral("показать ещё")
        || value.startsWith(QStringLiteral("ещё "))
        || value.startsWith(QStringLiteral("еще "))
        || value.startsWith(QStringLiteral("лиц."))
        || value.startsWith(QStringLiteral("до "))
        || value.startsWith(QStringLiteral("от "))
        || value.contains(QStringLiteral("отзыв"))
        || value.contains(QStringLiteral("₽"))
        || value.contains(QStringLiteral("%"))
        || value.contains(QStringLiteral("подобрано"))
        || value.contains(QStringLiteral("в москве"))
        || hasParserArtifact(line);
}

QString displayBankName(const QString &line)
{
    const QString candidate = normalizedLine(line);
    if (candidate.size() < 2 || candidate.size() > 70 || isServiceLine(candidate)) {
        return {};
    }

    const QString folded = candidate.toCaseFolded();
    if (folded.contains(QStringLiteral("другие вклады"))
        || folded.contains(QStringLiteral("другие кредиты"))
        || folded.contains(QStringLiteral("вклады банка"))
        || folded.contains(QStringLiteral("кредиты банка"))) {
        return {};
    }

    for (const BankAlias &alias : bankAliases()) {
        if (candidate.contains(alias.alias, Qt::CaseInsensitive)) {
            return alias.displayName;
        }
    }

    if (folded.contains(QStringLiteral("банк")) && !folded.contains(QStringLiteral("банки.ру"))) {
        return candidate.left(60);
    }
    return {};
}

double extractRateValue(const QString &line)
{
    static const QRegularExpression expression(
        QStringLiteral("(\\d{1,2}(?:[,.]\\d{1,3})?)\\s*(?:[–—-]\\s*\\d{1,2}(?:[,.]\\d{1,3})?)?\\s*%"));
    const QRegularExpressionMatch match = expression.match(line);
    if (!match.hasMatch()) {
        return 0.0;
    }

    QString value = match.captured(1);
    value.replace(',', '.');
    return value.toDouble();
}

int findLineIndex(const QStringList &lines, const QStringList &markers, int from = 0)
{
    for (int i = qMax(0, from); i < lines.size(); ++i) {
        const QString folded = lines[i].toCaseFolded();
        for (const QString &marker : markers) {
            if (folded.contains(marker.toCaseFolded())) {
                return i;
            }
        }
    }
    return -1;
}

QStringList htmlToLines(QString html)
{
    html.replace(QRegularExpression(QStringLiteral("<script[^>]*>.*?</script>"), QRegularExpression::DotMatchesEverythingOption), QStringLiteral(" "));
    html.replace(QRegularExpression(QStringLiteral("<style[^>]*>.*?</style>"), QRegularExpression::DotMatchesEverythingOption), QStringLiteral(" "));

    const QString plain = QTextDocumentFragment::fromHtml(html).toPlainText();
    QStringList result;
    for (const QString &rawLine : plain.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts)) {
        const QString line = normalizedLine(rawLine);
        if (!line.isEmpty()) {
            result.append(line);
        }
    }
    return result;
}

QString nextProductLine(const QStringList &lines, int bankIndex)
{
    for (int i = bankIndex + 1; i < qMin(lines.size(), bankIndex + 5); ++i) {
        const QString line = lines[i];
        if (!isServiceLine(line)) {
            return cleanedDescription(line);
        }
    }
    return {};
}

double findRateAfter(const QStringList &lines, int startIndex, int maxDistance, bool deposits)
{
    const int end = qMin(lines.size(), startIndex + maxDistance);
    for (int i = startIndex; i < end; ++i) {
        const QString folded = lines[i].toCaseFolded();
        if (!folded.contains(QStringLiteral("ставка"))) {
            continue;
        }

        const int scanStart = deposits ? qMax(startIndex, i - 2) : i;
        for (int j = scanStart; j < qMin(lines.size(), i + 4); ++j) {
            const double rate = extractRateValue(lines[j]);
            if (isPlausibleRate(rate, deposits)) {
                return rate;
            }
        }
    }
    return 0.0;
}

void appendRateIfUnique(QVector<RateRecord> *rates, const RateRecord &record)
{
    for (const RateRecord &existing : *rates) {
        if (existing.bank.compare(record.bank, Qt::CaseInsensitive) == 0) {
            return;
        }
    }
    rates->append(record);
}

QString sourceLabel(const QString &source)
{
    const QUrl url(source);
    return url.host().isEmpty() ? source : url.host();
}

QString jsonString(const QJsonValue &value)
{
    if (value.isString()) {
        return normalizedLine(value.toString());
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'f', 2);
    }
    return {};
}

double jsonNumber(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toDouble();
    }
    if (value.isString()) {
        QString text = value.toString();
        text.replace(',', '.');
        return text.toDouble();
    }
    return 0.0;
}

bool jsonTypeContains(const QJsonObject &object, const QString &type)
{
    const QJsonValue value = object.value(QStringLiteral("@type"));
    if (value.isString()) {
        return value.toString().compare(type, Qt::CaseInsensitive) == 0;
    }
    if (value.isArray()) {
        for (const QJsonValue &item : value.toArray()) {
            if (item.toString().compare(type, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
    }
    return false;
}

double rateFromJsonObject(const QJsonObject &object, bool deposits)
{
    const QJsonObject percentage = object.value(QStringLiteral("annualPercentageRate")).toObject();
    if (percentage.isEmpty()) {
        return 0.0;
    }

    const double preferred = deposits
        ? jsonNumber(percentage.value(QStringLiteral("maxValue")))
        : jsonNumber(percentage.value(QStringLiteral("minValue")));
    if (isPlausibleRate(preferred, deposits)) {
        return preferred;
    }

    const double fallback = deposits
        ? jsonNumber(percentage.value(QStringLiteral("minValue")))
        : jsonNumber(percentage.value(QStringLiteral("maxValue")));
    return isPlausibleRate(fallback, deposits) ? fallback : 0.0;
}

QString brokerNameFromJsonObject(const QJsonObject &object)
{
    const QStringList brokerKeys = {
        QStringLiteral("broker"),
        QStringLiteral("provider"),
        QStringLiteral("seller"),
        QStringLiteral("brand")
    };

    for (const QString &key : brokerKeys) {
        const QJsonValue value = object.value(key);
        if (value.isObject()) {
            const QString name = jsonString(value.toObject().value(QStringLiteral("name")));
            if (!name.isEmpty()) {
                return name;
            }
        } else if (value.isString()) {
            return jsonString(value);
        }
    }
    return {};
}

void collectJsonRates(const QJsonValue &value, bool deposits, const QString &source, const QString &fetchedAt, QVector<RateRecord> *rates)
{
    if (rates->size() >= 8) {
        return;
    }

    if (value.isArray()) {
        for (const QJsonValue &item : value.toArray()) {
            collectJsonRates(item, deposits, source, fetchedAt, rates);
            if (rates->size() >= 8) {
                return;
            }
        }
        return;
    }

    if (!value.isObject()) {
        return;
    }

    const QJsonObject object = value.toObject();
    const double rate = rateFromJsonObject(object, deposits);
    QString bank = displayBankName(brokerNameFromJsonObject(object));
    if (bank.isEmpty()) {
        bank = brokerNameFromJsonObject(object).left(60);
    }

    const bool expectedType = deposits
        ? (jsonTypeContains(object, QStringLiteral("DepositAccount")) || jsonTypeContains(object, QStringLiteral("BankAccount")))
        : jsonTypeContains(object, QStringLiteral("LoanOrCredit"));

    if (expectedType && isPlausibleRate(rate, deposits) && !isPlaceholderBank(bank)) {
        QString product = jsonString(object.value(QStringLiteral("name")));
        if (product.isEmpty()) {
            product = deposits ? QStringLiteral("Вклад или накопительный счет") : QStringLiteral("Потребительский кредит");
        }

        RateRecord record;
        record.bank = bank;
        record.rate = rate;
        record.description = cleanedDescription(QString("%1; ставка %2 %3%")
                                                    .arg(product,
                                                         deposits ? QStringLiteral("до") : QStringLiteral("от"))
                                                    .arg(rate, 0, 'f', 2));
        record.source = sourceLabel(source);
        record.fetchedAt = fetchedAt;
        if (!hasParserArtifact(record.description) && looksLikeProductDescription(record.description, deposits)) {
            appendRateIfUnique(rates, record);
        }
    }

    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        collectJsonRates(it.value(), deposits, source, fetchedAt, rates);
        if (rates->size() >= 8) {
            return;
        }
    }
}

QVector<RateRecord> parseJsonLdRates(const QString &html, const QString &source, const QString &fetchedAt, bool deposits)
{
    QVector<RateRecord> rates;
    static const QRegularExpression scriptExpression(
        QStringLiteral("<script[^>]*type=[\"']application/ld\\+json[\"'][^>]*>(.*?)</script>"),
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator iterator = scriptExpression.globalMatch(html);
    while (iterator.hasNext() && rates.size() < 8) {
        QString jsonText = iterator.next().captured(1).trimmed();
        if (jsonText.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            jsonText = QTextDocumentFragment::fromHtml(jsonText).toPlainText();
            const QJsonDocument repairedDocument = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                continue;
            }
            collectJsonRates(repairedDocument.isArray() ? QJsonValue(repairedDocument.array()) : QJsonValue(repairedDocument.object()),
                             deposits,
                             source,
                             fetchedAt,
                             &rates);
            continue;
        }

        collectJsonRates(document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object()),
                         deposits,
                         source,
                         fetchedAt,
                         &rates);
    }
    return rates;
}

QVector<RateRecord> parseCreditTextLines(const QStringList &lines, const QString &source, const QString &fetchedAt)
{
    QVector<RateRecord> rates;
    int start = findLineIndex(lines, { QStringLiteral("Подобрано"), QStringLiteral("Потребительские кредиты") });
    if (start < 0) {
        start = 0;
    }

    for (int i = start; i < lines.size() && rates.size() < 8; ++i) {
        const QString bank = displayBankName(lines[i]);
        if (bank.isEmpty()) {
            continue;
        }

        const double rate = findRateAfter(lines, i, 28, false);
        if (!isPlausibleRate(rate, false)) {
            continue;
        }

        QString product = nextProductLine(lines, i);
        if (product.isEmpty()) {
            product = QStringLiteral("Потребительский кредит");
        }

        RateRecord record;
        record.bank = bank;
        record.rate = rate;
        record.description = cleanedDescription(QStringLiteral("%1; ставка от %2%").arg(product).arg(rate, 0, 'f', 2));
        record.source = sourceLabel(source);
        record.fetchedAt = fetchedAt;
        appendRateIfUnique(&rates, record);
    }
    return rates;
}

QVector<RateRecord> parseDepositTextLines(const QStringList &lines, const QString &source, const QString &fetchedAt)
{
    QVector<RateRecord> rates;
    int start = findLineIndex(lines, { QStringLiteral("Все предложения"), QStringLiteral("По популярности") });
    if (start < 0) {
        start = 0;
    }
    int end = findLineIndex(lines, { QStringLiteral("Показать еще"), QStringLiteral("Показать ещё") }, start + 1);
    if (end < 0) {
        end = lines.size();
    }

    for (int i = start; i < end && rates.size() < 8; ++i) {
        const QString bank = displayBankName(lines[i]);
        if (bank.isEmpty()) {
            continue;
        }

        const double rate = findRateAfter(lines, i, 12, true);
        if (!isPlausibleRate(rate, true)) {
            continue;
        }

        QString product = nextProductLine(lines, i);
        if (product.isEmpty()) {
            product = QStringLiteral("Вклад или накопительный счет");
        }

        RateRecord record;
        record.bank = bank;
        record.rate = rate;
        record.description = cleanedDescription(QStringLiteral("%1; ставка до %2%").arg(product).arg(rate, 0, 'f', 2));
        record.source = sourceLabel(source);
        record.fetchedAt = fetchedAt;
        appendRateIfUnique(&rates, record);
    }
    return rates;
}

} // namespace

RatesProvider::RatesProvider(QObject *parent)
    : QObject(parent)
{
}

QVector<CurrencyRate> RatesProvider::updateCurrencyRates(QString *message)
{
    QString error;
    const QByteArray data = getUrl(QUrl(QStringLiteral("https://www.cbr.ru/scripts/XML_daily.asp")), &error);
    QVector<CurrencyRate> rates = parseCbrXml(data);
    if (!rates.isEmpty()) {
        DatabaseManager::instance().saveCurrencyRates(rates);
        if (message) {
            *message = QStringLiteral("Курсы валют обновлены из ЦБ РФ.");
        }
        return rates;
    }

    if (message) {
        *message = QString("Не удалось обновить данные из интернета. Показаны последние сохранённые данные. %1").arg(error);
    }
    return DatabaseManager::instance().loadCurrencyRates();
}

QVector<RateRecord> RatesProvider::updateCreditRates(QString *message)
{
    QStringList errors;
    const QList<QUrl> sources = {
        QUrl(QStringLiteral("https://www.banki.ru/products/credits/")),
        QUrl(QStringLiteral("https://www.sravni.ru/kredity/"))
    };

    for (const QUrl &url : sources) {
        QString error;
        QVector<RateRecord> rates = usableRates(parseRatesPage(getUrl(url, &error), url.toString(), false), false);
        if (hasEnoughRates(rates)) {
            DatabaseManager::instance().saveCreditRates(rates);
            if (message) {
                *message = QString("Ставки по кредитам обновлены из интернета. Источник: %1").arg(url.host());
            }
            return rates;
        }
        if (!error.isEmpty()) {
            errors.append(QString("%1: %2").arg(url.host(), error));
        } else {
            errors.append(QString("%1: получено слишком мало пригодных данных").arg(url.host()));
        }
    }

    const QVector<RateRecord> savedRates = usableRates(DatabaseManager::instance().loadCreditRates(), false);
    if (hasEnoughRates(savedRates)) {
        if (message) {
            *message = QStringLiteral("Не удалось получить полный список из интернета. Показаны последние сохранённые данные.");
        }
        return savedRates;
    }

    if (message) {
        const QString details = errors.isEmpty() ? QStringLiteral("Получено слишком мало пригодных данных.") : errors.join(QStringLiteral("; "));
        *message = QString("Не удалось обновить данные из интернета. %1").arg(details);
    }
    return {};
}

QVector<RateRecord> RatesProvider::updateDepositRates(QString *message)
{
    QStringList errors;
    const QList<QUrl> sources = {
        QUrl(QStringLiteral("https://www.banki.ru/products/deposits/")),
        QUrl(QStringLiteral("https://www.sravni.ru/vklady/"))
    };

    for (const QUrl &url : sources) {
        QString error;
        QVector<RateRecord> rates = usableRates(parseRatesPage(getUrl(url, &error), url.toString(), true), true);
        if (hasEnoughRates(rates)) {
            DatabaseManager::instance().saveDepositRates(rates);
            if (message) {
                *message = QString("Ставки по вкладам обновлены из интернета. Источник: %1").arg(url.host());
            }
            return rates;
        }
        if (!error.isEmpty()) {
            errors.append(QString("%1: %2").arg(url.host(), error));
        } else {
            errors.append(QString("%1: получено слишком мало пригодных данных").arg(url.host()));
        }
    }

    const QVector<RateRecord> savedRates = usableRates(DatabaseManager::instance().loadDepositRates(), true);
    if (hasEnoughRates(savedRates)) {
        if (message) {
            *message = QStringLiteral("Не удалось получить полный список из интернета. Показаны последние сохранённые данные.");
        }
        return savedRates;
    }

    if (message) {
        const QString details = errors.isEmpty() ? QStringLiteral("Получено слишком мало пригодных данных.") : errors.join(QStringLiteral("; "));
        *message = QString("Не удалось обновить данные из интернета. %1").arg(details);
    }
    return {};
}

QVector<CurrencyRate> RatesProvider::currencyRatesWithFallback(QString *message)
{
    QVector<CurrencyRate> rates = updateCurrencyRates(message);
    if (!rates.isEmpty()) {
        return rates;
    }
    const QString details = message ? message->trimmed() : QString();
    rates = demoCurrencyRates();
    if (message) {
        *message = details.isEmpty()
            ? QStringLiteral("Не удалось обновить данные из интернета. Сохранённых данных нет, показаны резервные данные для демонстрации.")
            : QString("%1 Сохранённых данных нет, показаны резервные данные для демонстрации.").arg(details);
    }
    return rates;
}

QVector<RateRecord> RatesProvider::creditRatesWithFallback(QString *message)
{
    QVector<RateRecord> rates = usableRates(updateCreditRates(message), false);
    if (hasEnoughRates(rates)) {
        return rates;
    }
    const QString details = message ? message->trimmed() : QString();
    rates = demoCreditRates();
    if (message) {
        *message = details.isEmpty()
            ? QStringLiteral("Не удалось обновить данные из интернета. Сохранённых данных нет, показаны резервные данные для демонстрации.")
            : QString("%1 Сохранённых данных нет, показаны резервные данные для демонстрации.").arg(details);
    }
    return rates;
}

QVector<RateRecord> RatesProvider::depositRatesWithFallback(QString *message)
{
    QVector<RateRecord> rates = usableRates(updateDepositRates(message), true);
    if (hasEnoughRates(rates)) {
        return rates;
    }
    const QString details = message ? message->trimmed() : QString();
    rates = demoDepositRates();
    if (message) {
        *message = details.isEmpty()
            ? QStringLiteral("Не удалось обновить данные из интернета. Сохранённых данных нет, показаны резервные данные для демонстрации.")
            : QString("%1 Сохранённых данных нет, показаны резервные данные для демонстрации.").arg(details);
    }
    return rates;
}

QByteArray RatesProvider::getUrl(const QUrl &url, QString *error)
{
    if (url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0 && !QSslSocket::supportsSsl()) {
        if (error) {
            *error = QStringLiteral("Qt не нашёл TLS/SSL runtime для HTTPS.");
        }
        return {};
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) MoneyShark/1.0 Qt"));
    request.setRawHeader("Accept-Language", "ru-RU,ru;q=0.9,en;q=0.7");
    QNetworkReply *reply = m_manager.get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(7000);
    loop.exec();

    QByteArray data;
    if (timer.isActive() && reply->error() == QNetworkReply::NoError) {
        data = reply->readAll();
    } else if (error) {
        *error = timer.isActive() ? reply->errorString() : QStringLiteral("Истекло время ожидания.");
    }

    reply->deleteLater();
    return data;
}

QVector<CurrencyRate> RatesProvider::parseCbrXml(const QByteArray &data) const
{
    QVector<CurrencyRate> rates;
    if (data.isEmpty()) {
        return rates;
    }

    const QString fetchedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    QXmlStreamReader xml(data);
    CurrencyRate current;
    bool inValute = false;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QStringLiteral("Valute")) {
            current = CurrencyRate();
            current.source = QStringLiteral("ЦБ РФ");
            current.fetchedAt = fetchedAt;
            inValute = true;
        } else if (xml.isEndElement() && xml.name() == QStringLiteral("Valute")) {
            if (current.code == QStringLiteral("USD")
                || current.code == QStringLiteral("EUR")
                || current.code == QStringLiteral("CNY")) {
                rates.append(current);
            }
            inValute = false;
        } else if (inValute && xml.isStartElement()) {
            const QString elementName = xml.name().toString();
            const QString text = xml.readElementText();
            if (elementName == QStringLiteral("CharCode")) {
                current.code = text;
            } else if (elementName == QStringLiteral("Name")) {
                current.name = text;
            } else if (elementName == QStringLiteral("Value")) {
                QString valueText = text;
                valueText.replace(',', '.');
                current.value = valueText.toDouble();
            }
        }
    }
    return rates;
}

QVector<RateRecord> RatesProvider::parseRatesPage(const QByteArray &data, const QString &source, bool deposits) const
{
    QVector<RateRecord> rates;
    if (data.isEmpty()) {
        return rates;
    }

    const QString fetchedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    const QString html = QString::fromUtf8(data);
    rates = parseJsonLdRates(html, source, fetchedAt, deposits);
    if (hasEnoughRates(rates)) {
        return rates;
    }

    const QStringList lines = htmlToLines(html);
    return deposits
        ? parseDepositTextLines(lines, source, fetchedAt)
        : parseCreditTextLines(lines, source, fetchedAt);
}

QVector<RateRecord> RatesProvider::demoCreditRates() const
{
    const QString fetchedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    return {
        { -1, QStringLiteral("Сбер"), 14.9, QStringLiteral("Резервные данные для демонстрации: потребительский кредит"), QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("ВТБ"), 15.5, QStringLiteral("Резервные данные для демонстрации: кредит наличными"), QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("Альфа-Банк"), 15.9, QStringLiteral("Резервные данные для демонстрации: индивидуальные условия"), QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("Т-Банк"), 13.9, QStringLiteral("Резервные данные для демонстрации: заявка онлайн"), QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("Газпромбанк"), 16.5, QStringLiteral("Резервные данные для демонстрации: требуется одобрение"), QStringLiteral("резервные данные"), fetchedAt }
    };
}

QVector<RateRecord> RatesProvider::demoDepositRates() const
{
    const QString fetchedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    return {
        { -1, QStringLiteral("Газпромбанк"), 12.5, QStringLiteral("Резервные данные для демонстрации: вклад на 1 год"), QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("Сбер"), 12.0, QStringLiteral("Резервные данные для демонстрации: вклад с капитализацией"), QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("ВТБ"), 11.8, QStringLiteral("Резервные данные для демонстрации: онлайн-вклад"), QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("Альфа-Банк"), 11.5, QStringLiteral("Резервные данные для демонстрации: срочный вклад"), QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("Т-Банк"), 11.2, QStringLiteral("Резервные данные для демонстрации: накопительный продукт"), QStringLiteral("резервные данные"), fetchedAt }
    };
}

QVector<CurrencyRate> RatesProvider::demoCurrencyRates() const
{
    const QString fetchedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    return {
        { -1, QStringLiteral("USD"), QStringLiteral("Доллар США"), 92.50, QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("EUR"), QStringLiteral("Евро"), 100.20, QStringLiteral("резервные данные"), fetchedAt },
        { -1, QStringLiteral("CNY"), QStringLiteral("Китайский юань"), 12.80, QStringLiteral("резервные данные"), fetchedAt }
    };
}
