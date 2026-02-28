#include "mainwindow.h"
#include "connectiondialog.h"
#include "creditcalculator.h"
#include "database.h"
#include "depositcalculator.h"
#include "financemanager.h"
#include "financewidget.h"
#include "logo.h"
#include "statisticswidget.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QSettings>
#include <QSizePolicy>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(900, 650);
    resize(1100, 760);
    setWindowIcon(moneySharkIcon());
    m_currentBalance = DatabaseManager::instance().loadBalance();
    setupMenuBar();
    setupStackedWidget();
    updateBalanceDisplay();
    setMainTitle();
    statusBar()->showMessage(DatabaseManager::instance().connectionDescription());
}

MainWindow::~MainWindow()
{
    DatabaseManager::instance().saveBalance(m_currentBalance);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("Файл"));
    QAction *connectionAction = fileMenu->addAction(QStringLiteral("Настройки подключения"));
    QAction *exitAction = fileMenu->addAction(QStringLiteral("Выход"));

    QMenu *dataMenu = menuBar()->addMenu(QStringLiteral("Данные"));
    QAction *currencyAction = dataMenu->addAction(QStringLiteral("Курс валют"));
    QAction *creditRatesAction = dataMenu->addAction(QStringLiteral("Проценты по кредитам"));
    QAction *depositRatesAction = dataMenu->addAction(QStringLiteral("Лучший процент по вкладам"));

    QMenu *userMenu = menuBar()->addMenu(QString("Пользователь: %1").arg(DatabaseManager::instance().currentLogin()));
    QAction *modeAction = userMenu->addAction(DatabaseManager::instance().connectionDescription());
    modeAction->setEnabled(false);

    connect(connectionAction, &QAction::triggered, this, &MainWindow::openConnectionSettings);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    connect(currencyAction, &QAction::triggered, this, &MainWindow::showCurrencyRates);
    connect(creditRatesAction, &QAction::triggered, this, &MainWindow::showCreditRates);
    connect(depositRatesAction, &QAction::triggered, this, &MainWindow::showDepositRates);
}

void MainWindow::setupStackedWidget()
{
    m_stackedWidget = new QStackedWidget(this);
    m_mainPage = createMainPage();

    QPushButton *manualFinanceButton = makeMenuButton(QStringLiteral("Ручная запись финансов"));
    QPushButton *statisticsButton = makeMenuButton(QStringLiteral("Статистика доходов и расходов"));
    QPushButton *currencyButton = makeMenuButton(QStringLiteral("Курс валют"));
    QPushButton *addMoneyButton = makeMenuButton(QStringLiteral("+1000 руб."));
    QPushButton *subtractMoneyButton = makeMenuButton(QStringLiteral("-500 руб."));
    QPushButton *customMoneyButton = makeMenuButton(QStringLiteral("Своя сумма"));
    m_subMenuFinance = createSubMenu(
        QStringLiteral("Финансы"),
        {manualFinanceButton, statisticsButton, currencyButton, addMoneyButton, subtractMoneyButton, customMoneyButton},
        QStringLiteral("backButtonFinance"));

    QPushButton *creditRatesButton = makeMenuButton(QStringLiteral("Проценты по кредитам"));
    QPushButton *creditCalculatorButton = makeMenuButton(QStringLiteral("Кредитный калькулятор"));
    QPushButton *creditBanksButton = makeMenuButton(QStringLiteral("Название банка"));
    m_subMenuCredits = createSubMenu(
        QStringLiteral("Кредиты"),
        {creditRatesButton, creditCalculatorButton, creditBanksButton},
        QStringLiteral("backButtonCredits"));

    QPushButton *depositRatesButton = makeMenuButton(QStringLiteral("Лучший процент"));
    QPushButton *depositCalculatorButton = makeMenuButton(QStringLiteral("Калькулятор доходности"));
    QPushButton *depositBanksButton = makeMenuButton(QStringLiteral("Название банков"));
    m_subMenuDeposits = createSubMenu(
        QStringLiteral("Вклады"),
        {depositRatesButton, depositCalculatorButton, depositBanksButton},
        QStringLiteral("backButtonDeposits"));

    connect(manualFinanceButton, &QPushButton::clicked, this, &MainWindow::openFinanceWidget);
    connect(statisticsButton, &QPushButton::clicked, this, &MainWindow::openStatisticsWidget);
    connect(currencyButton, &QPushButton::clicked, this, &MainWindow::showCurrencyRates);
    connect(addMoneyButton, &QPushButton::clicked, this, &MainWindow::on_addMoney_clicked);
    connect(subtractMoneyButton, &QPushButton::clicked, this, &MainWindow::on_subtractMoney_clicked);
    connect(customMoneyButton, &QPushButton::clicked, this, &MainWindow::on_setCustomMoney_clicked);
    connect(creditRatesButton, &QPushButton::clicked, this, &MainWindow::showCreditRates);
    connect(creditCalculatorButton, &QPushButton::clicked, this, &MainWindow::openCreditCalculator);
    connect(creditBanksButton, &QPushButton::clicked, this, &MainWindow::showCreditBanks);
    connect(depositRatesButton, &QPushButton::clicked, this, &MainWindow::showDepositRates);
    connect(depositCalculatorButton, &QPushButton::clicked, this, &MainWindow::openDepositCalculator);
    connect(depositBanksButton, &QPushButton::clicked, this, &MainWindow::showDepositBanks);

    connect(m_subMenuFinance->findChild<QPushButton*>(QStringLiteral("backButtonFinance")), &QPushButton::clicked, this, &MainWindow::on_backToMain_1_clicked);
    connect(m_subMenuCredits->findChild<QPushButton*>(QStringLiteral("backButtonCredits")), &QPushButton::clicked, this, &MainWindow::on_backToMain_2_clicked);
    connect(m_subMenuDeposits->findChild<QPushButton*>(QStringLiteral("backButtonDeposits")), &QPushButton::clicked, this, &MainWindow::on_backToMain_3_clicked);

    m_stackedWidget->addWidget(m_mainPage);
    m_stackedWidget->addWidget(m_subMenuFinance);
    m_stackedWidget->addWidget(m_subMenuCredits);
    m_stackedWidget->addWidget(m_subMenuDeposits);
    setCentralWidget(m_stackedWidget);
}

QWidget *MainWindow::createMainPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(34, 28, 34, 34);
    layout->setSpacing(18);

    QHBoxLayout *topLayout = new QHBoxLayout();
    m_userLabel = new QLabel(
        QString("Пользователь: %1 | %2")
            .arg(DatabaseManager::instance().currentLogin(), DatabaseManager::instance().connectionDescription()),
        page);
    m_userLabel->setWordWrap(true);
    m_balanceLabel = new QLabel(page);
    m_balanceLabel->setObjectName(QStringLiteral("balanceLabel"));
    m_balanceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    topLayout->addWidget(m_userLabel, 1);
    topLayout->addWidget(m_balanceLabel);
    layout->addLayout(topLayout);

    MoneySharkLogoWidget *logoWidget = new MoneySharkLogoWidget(page);
    logoWidget->setMaximumHeight(230);
    layout->addWidget(logoWidget, 0, Qt::AlignHCenter);

    QLabel *titleLabel = new QLabel(QStringLiteral("Money Shark"), page);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QPushButton *financeButton = makeMenuButton(QStringLiteral("Money Shark - Финансы"));
    financeButton->setObjectName(QStringLiteral("pushButton_Finansy"));
    QPushButton *creditsButton = makeMenuButton(QStringLiteral("Money Shark - Кредиты"));
    creditsButton->setObjectName(QStringLiteral("pushButton_Kredity"));
    QPushButton *depositsButton = makeMenuButton(QStringLiteral("Money Shark - Вклады"));
    depositsButton->setObjectName(QStringLiteral("pushButton_Vklady"));

    layout->addStretch(1);
    layout->addWidget(financeButton);
    layout->addWidget(creditsButton);
    layout->addWidget(depositsButton);
    layout->addStretch(2);

    connect(financeButton, &QPushButton::clicked, this, &MainWindow::on_pushButton_Finansy_clicked);
    connect(creditsButton, &QPushButton::clicked, this, &MainWindow::on_pushButton_Kredity_clicked);
    connect(depositsButton, &QPushButton::clicked, this, &MainWindow::on_pushButton_Vklady_clicked);
    return page;
}

QWidget *MainWindow::createSubMenu(const QString &title, const QList<QPushButton*> &buttons, const QString &backButtonObjectName)
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(34, 28, 34, 34);
    layout->setSpacing(16);

    QLabel *titleLabel = new QLabel(title, page);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    layout->addStretch(1);
    for (QPushButton *button : buttons) {
        button->setParent(page);
        layout->addWidget(button);
    }
    layout->addStretch(2);

    QPushButton *backButton = makeMenuButton(QStringLiteral("Назад в главное меню"));
    backButton->setObjectName(backButtonObjectName);
    backButton->setProperty("secondary", true);
    backButton->setParent(page);
    layout->addWidget(backButton);
    return page;
}

QPushButton *MainWindow::makeMenuButton(const QString &text)
{
    QPushButton *button = new QPushButton(text);
    button->setMinimumHeight(58);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return button;
}

void MainWindow::updateBalanceDisplay()
{
    if (m_balanceLabel) {
        m_balanceLabel->setText(QString("Баланс: %1 руб.").arg(m_currentBalance, 0, 'f', 2));
    }
}

void MainWindow::setMainTitle()
{
    setWindowTitle(QString("Money Shark - %1").arg(DatabaseManager::instance().currentLogin()));
}

void MainWindow::on_pushButton_Finansy_clicked()
{
    m_stackedWidget->setCurrentWidget(m_subMenuFinance);
    setWindowTitle(QStringLiteral("Money Shark - Финансы"));
}

void MainWindow::on_pushButton_Kredity_clicked()
{
    m_stackedWidget->setCurrentWidget(m_subMenuCredits);
    setWindowTitle(QStringLiteral("Money Shark - Кредиты"));
}

void MainWindow::on_pushButton_Vklady_clicked()
{
    m_stackedWidget->setCurrentWidget(m_subMenuDeposits);
    setWindowTitle(QStringLiteral("Money Shark - Вклады"));
}

void MainWindow::on_backToMain_1_clicked()
{
    m_stackedWidget->setCurrentWidget(m_mainPage);
    setMainTitle();
}

void MainWindow::on_backToMain_2_clicked()
{
    on_backToMain_1_clicked();
}

void MainWindow::on_backToMain_3_clicked()
{
    on_backToMain_1_clicked();
}

void MainWindow::on_addMoney_clicked()
{
    m_currentBalance += 1000.0;
    DatabaseManager::instance().saveBalance(m_currentBalance);
    updateBalanceDisplay();
    QMessageBox::information(this, QStringLiteral("Баланс"), QString("Баланс пополнен на 1000 руб.\nНовый баланс: %1 руб.").arg(m_currentBalance, 0, 'f', 2));
}

void MainWindow::on_subtractMoney_clicked()
{
    if (m_currentBalance < 500.0) {
        QMessageBox::warning(this, QStringLiteral("Баланс"), QString("Недостаточно средств.\nТекущий баланс: %1 руб.").arg(m_currentBalance, 0, 'f', 2));
        return;
    }
    m_currentBalance -= 500.0;
    DatabaseManager::instance().saveBalance(m_currentBalance);
    updateBalanceDisplay();
    QMessageBox::information(this, QStringLiteral("Баланс"), QString("Списано 500 руб.\nНовый баланс: %1 руб.").arg(m_currentBalance, 0, 'f', 2));
}

void MainWindow::on_setCustomMoney_clicked()
{
    bool ok = false;
    const double amount = QInputDialog::getDouble(
        this,
        QStringLiteral("Своя сумма"),
        QStringLiteral("Введите новый баланс, руб.:"),
        m_currentBalance,
        0.0,
        1000000000.0,
        2,
        &ok);
    if (!ok) {
        return;
    }
    m_currentBalance = amount;
    DatabaseManager::instance().saveBalance(m_currentBalance);
    updateBalanceDisplay();
}

void MainWindow::openFinanceWidget()
{
    if (!m_financeWidget) {
        m_financeWidget = new FinanceWidget(this, &m_currentBalance);
        m_stackedWidget->addWidget(m_financeWidget);
        connect(m_financeWidget, &FinanceWidget::backToMenu, this, [this]() {
            m_stackedWidget->setCurrentWidget(m_subMenuFinance);
            updateBalanceDisplay();
            setWindowTitle(QStringLiteral("Money Shark - Финансы"));
        });
        connect(m_financeWidget, &FinanceWidget::balanceUpdated, this, [this](double balance) {
            m_currentBalance = balance;
            updateBalanceDisplay();
        });
    }
    QMetaObject::invokeMethod(m_financeWidget, "refreshUI");
    m_stackedWidget->setCurrentWidget(m_financeWidget);
    setWindowTitle(QStringLiteral("Money Shark - Ручная запись финансов"));
}

void MainWindow::openStatisticsWidget()
{
    if (!m_statisticsWidget) {
        m_statisticsWidget = new StatisticsWidget(this);
        m_stackedWidget->addWidget(m_statisticsWidget);
        connect(m_statisticsWidget, &StatisticsWidget::backToMenu, this, [this]() {
            m_stackedWidget->setCurrentWidget(m_subMenuFinance);
            setWindowTitle(QStringLiteral("Money Shark - Финансы"));
        });
    }
    QMetaObject::invokeMethod(m_statisticsWidget, "refreshStats");
    m_stackedWidget->setCurrentWidget(m_statisticsWidget);
    setWindowTitle(QStringLiteral("Money Shark - Статистика"));
}

void MainWindow::openCreditCalculator()
{
    if (!m_creditCalculatorWidget) {
        m_creditCalculatorWidget = new CreditCalculator(this);
        m_stackedWidget->addWidget(m_creditCalculatorWidget);
        connect(m_creditCalculatorWidget, &CreditCalculator::backToMenu, this, [this]() {
            m_stackedWidget->setCurrentWidget(m_subMenuCredits);
            setWindowTitle(QStringLiteral("Money Shark - Кредиты"));
        });
    }
    m_stackedWidget->setCurrentWidget(m_creditCalculatorWidget);
    setWindowTitle(QStringLiteral("Money Shark - Кредитный калькулятор"));
}

void MainWindow::openDepositCalculator()
{
    if (!m_depositCalculatorWidget) {
        m_depositCalculatorWidget = new DepositCalculator(this);
        m_stackedWidget->addWidget(m_depositCalculatorWidget);
        connect(m_depositCalculatorWidget, &DepositCalculator::backToMenu, this, [this]() {
            m_stackedWidget->setCurrentWidget(m_subMenuDeposits);
            setWindowTitle(QStringLiteral("Money Shark - Вклады"));
        });
    }
    m_stackedWidget->setCurrentWidget(m_depositCalculatorWidget);
    setWindowTitle(QStringLiteral("Money Shark - Калькулятор доходности"));
}

void MainWindow::showCurrencyRates()
{
    statusBar()->showMessage(QStringLiteral("Обновление данных..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    QString message;
    const QVector<CurrencyRate> rates = m_ratesProvider.currencyRatesWithFallback(&message);
    QApplication::restoreOverrideCursor();
    statusBar()->showMessage(DatabaseManager::instance().connectionDescription());
    showCurrencyTable(rates, message);
}

void MainWindow::showCreditRates()
{
    statusBar()->showMessage(QStringLiteral("Обновление данных..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    QString message;
    const QVector<RateRecord> rates = m_ratesProvider.creditRatesWithFallback(&message);
    QApplication::restoreOverrideCursor();
    statusBar()->showMessage(DatabaseManager::instance().connectionDescription());
    showRatesTable(QStringLiteral("Проценты по кредитам"), rates, message);
}

void MainWindow::showDepositRates()
{
    statusBar()->showMessage(QStringLiteral("Обновление данных..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    QString message;
    const QVector<RateRecord> rates = m_ratesProvider.depositRatesWithFallback(&message);
    QApplication::restoreOverrideCursor();
    statusBar()->showMessage(DatabaseManager::instance().connectionDescription());
    showRatesTable(QStringLiteral("Лучший процент по вкладам"), rates, message);
}

void MainWindow::showCreditBanks()
{
    statusBar()->showMessage(QStringLiteral("Обновление данных..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    QString message;
    const QVector<RateRecord> rates = m_ratesProvider.creditRatesWithFallback(&message);
    QApplication::restoreOverrideCursor();
    statusBar()->showMessage(DatabaseManager::instance().connectionDescription());
    showBankList(QStringLiteral("Банки для кредитования"), rates, message);
}

void MainWindow::showDepositBanks()
{
    statusBar()->showMessage(QStringLiteral("Обновление данных..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    QString message;
    const QVector<RateRecord> rates = m_ratesProvider.depositRatesWithFallback(&message);
    QApplication::restoreOverrideCursor();
    statusBar()->showMessage(DatabaseManager::instance().connectionDescription());
    showBankList(QStringLiteral("Банки для вкладов"), rates, message);
}

void MainWindow::openConnectionSettings()
{
    ConnectionDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QMessageBox::information(
        this,
        QStringLiteral("Настройки подключения"),
        QStringLiteral("Настройки сохранены. Они будут использованы при следующем запуске приложения."));
}

void MainWindow::showRatesTable(const QString &title, const QVector<RateRecord> &rates, const QString &message)
{
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.resize(850, 520);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *messageLabel = new QLabel(message, &dialog);
    messageLabel->setWordWrap(true);
    layout->addWidget(messageLabel);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({
        QStringLiteral("Банк"),
        QStringLiteral("Ставка, %"),
        QStringLiteral("Описание"),
        QStringLiteral("Источник"),
        QStringLiteral("Дата обновления")
    });
    table->setRowCount(rates.size());
    for (int row = 0; row < rates.size(); ++row) {
        const RateRecord &rate = rates[row];
        table->setItem(row, 0, new QTableWidgetItem(rate.bank));
        table->setItem(row, 1, new QTableWidgetItem(QString::number(rate.rate, 'f', 2)));
        table->setItem(row, 2, new QTableWidgetItem(rate.description));
        table->setItem(row, 3, new QTableWidgetItem(rate.source));
        table->setItem(row, 4, new QTableWidgetItem(rate.fetchedAt));
    }
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->resizeColumnsToContents();
    layout->addWidget(table, 1);

    QPushButton *closeButton = new QPushButton(QStringLiteral("Закрыть"), &dialog);
    closeButton->setProperty("secondary", true);
    layout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

void MainWindow::showCurrencyTable(const QVector<CurrencyRate> &rates, const QString &message)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Курс валют"));
    dialog.resize(760, 420);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *messageLabel = new QLabel(message, &dialog);
    messageLabel->setWordWrap(true);
    layout->addWidget(messageLabel);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({
        QStringLiteral("Код"),
        QStringLiteral("Название"),
        QStringLiteral("Курс, руб."),
        QStringLiteral("Источник"),
        QStringLiteral("Дата обновления")
    });
    table->setRowCount(rates.size());
    for (int row = 0; row < rates.size(); ++row) {
        const CurrencyRate &rate = rates[row];
        table->setItem(row, 0, new QTableWidgetItem(rate.code));
        table->setItem(row, 1, new QTableWidgetItem(rate.name));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(rate.value, 'f', 4)));
        table->setItem(row, 3, new QTableWidgetItem(rate.source));
        table->setItem(row, 4, new QTableWidgetItem(rate.fetchedAt));
    }
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->resizeColumnsToContents();
    layout->addWidget(table, 1);

    QPushButton *closeButton = new QPushButton(QStringLiteral("Закрыть"), &dialog);
    closeButton->setProperty("secondary", true);
    layout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

void MainWindow::showBankList(const QString &title, const QVector<RateRecord> &rates, const QString &message)
{
    QVector<RateRecord> displayRates;
    QStringList seenBanks;
    for (const RateRecord &rate : rates) {
        const QString bank = rate.bank.trimmed();
        const QString normalized = bank.toCaseFolded();
        if (bank.isEmpty()
            || normalized == QStringLiteral("банк по кредиту")
            || normalized == QStringLiteral("банк по вкладу")
            || seenBanks.contains(normalized)) {
            continue;
        }
        seenBanks.append(normalized);
        displayRates.append(rate);
    }

    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.resize(760, 420);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *messageLabel = new QLabel(message, &dialog);
    messageLabel->setWordWrap(true);
    layout->addWidget(messageLabel);

    if (displayRates.isEmpty()) {
        QLabel *emptyLabel = new QLabel(QStringLiteral("Нет сохранённых данных о банках."), &dialog);
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel, 1);
    } else {
        QTableWidget *table = new QTableWidget(&dialog);
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({
            QStringLiteral("Банк"),
            QStringLiteral("Ставка, %"),
            QStringLiteral("Описание"),
            QStringLiteral("Источник")
        });
        table->setRowCount(displayRates.size());
        for (int row = 0; row < displayRates.size(); ++row) {
            const RateRecord &rate = displayRates[row];
            table->setItem(row, 0, new QTableWidgetItem(rate.bank));
            table->setItem(row, 1, new QTableWidgetItem(QString::number(rate.rate, 'f', 2)));
            table->setItem(row, 2, new QTableWidgetItem(rate.description));
            table->setItem(row, 3, new QTableWidgetItem(rate.source));
        }
        table->horizontalHeader()->setStretchLastSection(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->resizeColumnsToContents();
        layout->addWidget(table, 1);
    }

    QPushButton *closeButton = new QPushButton(QStringLiteral("Закрыть"), &dialog);
    closeButton->setProperty("secondary", true);
    layout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

void MainWindow::reloadRatesSilently()
{
    QString ignored;
    m_ratesProvider.updateCurrencyRates(&ignored);
    m_ratesProvider.updateCreditRates(&ignored);
    m_ratesProvider.updateDepositRates(&ignored);
}
