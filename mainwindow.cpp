#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QBrush>
#include <QByteArray>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFormLayout>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QRandomGenerator>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSettings>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStyle>
#include <QTableWidgetItem>
#include <QTableWidget>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {
const QString PesoSymbol = QString::fromUtf8("\xE2\x82\xB1");

bool isInsideWidget(QWidget *candidate, QWidget *ancestor)
{
    while (candidate) {
        if (candidate == ancestor) {
            return true;
        }
        candidate = candidate->parentWidget();
    }
    return false;
}

}

class CompoundInterestChart : public QWidget {
public:
    explicit CompoundInterestChart(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(260);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setProjection(double principalAmount, double annualRatePercent)
    {
        principal = std::max(0.0, principalAmount);
        annualRate = std::max(0.0, annualRatePercent);
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QRectF bounds = rect().adjusted(4, 4, -4, -4);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#F7FAFC"));
        painter.drawRoundedRect(bounds, 8, 8);

        QRectF plot = bounds.adjusted(46, 24, -24, -46);
        painter.setPen(QPen(QColor("#D7E0EA"), 1));
        painter.drawLine(plot.bottomLeft(), plot.bottomRight());
        painter.drawLine(plot.bottomLeft(), plot.topLeft());

        if (principal <= 0.0) {
            painter.setPen(QColor("#607086"));
            painter.setFont(QFont("Segoe UI", 9, QFont::DemiBold));
            painter.drawText(bounds, Qt::AlignCenter, "No projection data yet");
            return;
        }

        const QVector<int> years = {5, 10, 20, 40};
        QVector<double> values;
        double maxValue = principal;
        for (int year : years) {
            const double value = principal * std::pow(1.0 + (annualRate / 100.0), year);
            values.append(value);
            maxValue = std::max(maxValue, value);
        }

        painter.setPen(QPen(QColor("#E2E8F0"), 1, Qt::DashLine));
        const double capitalY = plot.bottom() - plot.height() * (principal / maxValue);
        painter.drawLine(QPointF(plot.left(), capitalY), QPointF(plot.right(), capitalY));
        painter.setPen(QColor("#607086"));
        painter.setFont(QFont("Segoe UI", 8, QFont::DemiBold));
        painter.drawText(QRectF(plot.left(), capitalY - 18, 150, 16),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         "Capital " + compactCurrency(principal));

        const double segment = plot.width() / values.size();
        const double barWidth = std::min(86.0, segment * 0.48);
        for (int i = 0; i < values.size(); ++i) {
            const double ratio = maxValue <= 0.0 ? 0.0 : values[i] / maxValue;
            const double barHeight = plot.height() * ratio;
            const double x = plot.left() + segment * i + (segment - barWidth) / 2.0;
            const QRectF bar(x, plot.bottom() - barHeight, barWidth, barHeight);

            QColor fill = i == 0 ? QColor("#8AB4F8")
                         : i == 1 ? QColor("#2E6FBF")
                         : i == 2 ? QColor("#1F8A70")
                                  : QColor("#F2B84B");
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawRoundedRect(bar, 8, 8);

            painter.setPen(QColor("#1F2A37"));
            painter.setFont(QFont("Segoe UI", 8, QFont::Bold));
            painter.drawText(QRectF(x - 24, bar.top() - 22, barWidth + 48, 18),
                             Qt::AlignCenter,
                             compactCurrency(values[i]));

            painter.setPen(QColor("#607086"));
            painter.setFont(QFont("Segoe UI", 9, QFont::DemiBold));
            painter.drawText(QRectF(x - 10, plot.bottom() + 8, barWidth + 20, 20),
                             Qt::AlignCenter,
                             QString::number(years[i]) + " yrs");
        }
    }

private:
    QString compactCurrency(double value) const
    {
        if (value >= 1000000.0) {
            return PesoSymbol + QString::number(value / 1000000.0, 'f', 2) + "M";
        }
        if (value >= 1000.0) {
            return PesoSymbol + QString::number(value / 1000.0, 'f', 1) + "K";
        }
        return PesoSymbol + QString::number(value, 'f', 0);
    }

    double principal = 0.0;
    double annualRate = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Constructor — wires all signals & slots, sets up table headers and filters
// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    lightThemeStyle = buildLightThemeStyle();
    setStyleSheet(lightThemeStyle);
    setWindowTitle("BankTracker - Banking & Investment Tracker");
    setupModernUi();

    QStringList headers = {"Card Number", "Name", "Username", "Account Type",
                           "Savings (" + PesoSymbol + ")", "Interest Rate", "Status"};
    ui->tableWidget->setColumnCount(headers.size());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setWordWrap(false);
    ui->tableWidget->setTextElideMode(Qt::ElideRight);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(44);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(false);
    ui->tableWidget->horizontalHeader()->setMinimumSectionSize(92);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget->setColumnWidth(0, 154);
    ui->tableWidget->setColumnWidth(2, 132);
    ui->tableWidget->setColumnWidth(3, 130);
    ui->tableWidget->setColumnWidth(4, 122);
    ui->tableWidget->setColumnWidth(5, 116);
    ui->tableWidget->setColumnWidth(6, 110);

    // ── Category filter options (Aaron) ───────────────────────────────────
    ui->categoryFilter->addItems({"All", "Savings", "Stocks", "Time Deposit", "UITF"});
    ui->categoryFilter->setInsertPolicy(QComboBox::NoInsert);
    ui->categoryFilter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->categoryFilter->view()->setMinimumWidth(180);
    ui->categoryFilter->setMaxVisibleItems(8);

    // ── Account type combo (Aaron) ─────────────────────────────────────────
    ui->comboAccountType->addItems({"Savings", "Stocks", "Time Deposit", "UITF"});
    ui->comboStatus->addItems({"Active", "Inactive", "Suspended"});
    ui->sortCombo->addItems({"Sort: Latest", "Alphabetical", "Savings: High to Low",
                             "Username", "Status"});
    applyComboPopupTheme(false);

    ui->contentLayout->setStretch(3, 1);
    ui->mainWorkspaceLayout->setStretch(0, 1);
    ui->mainWorkspaceLayout->setStretch(1, 0);
    setActiveNav(ui->btnDashboard);
    ui->screenSubtitle->setText("Monitor account savings, account status, and administrative activity.");

    ui->inputCardNumber->setInputMask("9999 9999 9999 9999;_");
    ui->inputSavings->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->spinInterest->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->inputSavings->setGroupSeparatorShown(true);
    ui->inputSavings->setSingleStep(1000.0);
    ui->spinInterest->setSingleStep(0.25);
    ui->inputSavings->setAlignment(Qt::AlignRight);
    ui->spinInterest->setAlignment(Qt::AlignRight);
    ui->inputSavings->setSpecialValueText("");
    ui->spinInterest->setReadOnly(true);
    ui->spinInterest->setSpecialValueText("Auto");
    ui->spinInterest->setFocusPolicy(Qt::NoFocus);
    ui->spinInterest->setToolTip("Calculated automatically from account savings.");
    ui->inputInvestments->hide();
    ui->spinMonths->hide();
    ui->statusLegendActive->hide();
    ui->statusLegendInactive->hide();
    ui->statusLegendSuspended->hide();

    const QList<QWidget *> focusWidgets = {
        ui->searchBar,
        ui->categoryFilter,
        ui->sortCombo,
        ui->inputName,
        ui->inputUsername,
        ui->inputCardNumber,
        ui->comboAccountType,
        ui->comboStatus,
        ui->inputSavings
    };
    for (QWidget *widget : focusWidgets) {
        widget->setFocusPolicy(Qt::StrongFocus);
        widget->installEventFilter(this);
    }

    connect(ui->comboAccountType, &QComboBox::currentTextChanged,
            this, &MainWindow::updateAutomatedInterest);
    connect(ui->inputSavings, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { updateAutomatedInterest(); updateInvestmentChart(); });
    setFormMode(false);
    setRightRailVisible(false);
    setupMainViews();
    showAccountsView();
    qApp->installEventFilter(this);

    // ── Initial summary cards (Reindel) ───────────────────────────────────
    loadAccounts();
    updateSummaryCards();
    populateTable();
    updateAutomatedInterest();
    updateInvestmentChart();
}

MainWindow::~MainWindow()
{
    qApp->removeEventFilter(this);
    saveAccounts();
    delete ui;
}


// ─────────────────────────────────────────────────────────────────────────────
// on_btnAdd_clicked — creates a new account or saves the selected account edit
// Owner: Keith
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_btnAdd_clicked()
{
    // Read all fields
    QString name        = ui->inputName->text().trimmed();
    QString userName    = ui->inputUsername->text().trimmed();
    QString cardNumber  = ui->inputCardNumber->text().trimmed();
    QString compactCard = cardNumber;
    compactCard.remove(' ');
    QString accountType = ui->comboAccountType->currentText();
    double  savings     = ui->inputSavings->value();
    double  investments = 0.0;
    int     months      = 0;
    double  interest    = fixedInterestRate(savings);
    ui->spinInterest->setValue(interest);
    QString status      = ui->comboStatus->currentText().isEmpty()
                         ? computeStatus(savings)
                         : ui->comboStatus->currentText();

    if (name.isEmpty() || userName.isEmpty()
        || compactCard.size() != 16 || compactCard.contains('_')) {
        QMessageBox::warning(this, "Input Error",
                             "Please enter the full name, user name, and complete card number.");
        return;
    }

    for (int i = 0; i < accounts.size(); ++i) {
        if (i != editingAccountIndex && accounts[i].cardNumber == cardNumber) {
            QMessageBox::warning(this, "Duplicate Card",
                                 "An account with this card number already exists.");
            return;
        }
    }

    // Build struct and add to QVector
    AccountHolder holder;
    holder.cardNumber     = cardNumber;
    holder.name           = name;
    holder.userName       = userName;
    holder.accountType    = accountType;
    holder.savings        = savings;
    holder.investments    = investments;
    holder.monthsInvested = months;
    holder.interestRate   = interest;
    holder.status         = status;

    if (editingAccountIndex >= 0 && editingAccountIndex < accounts.size()) {
        accounts[editingAccountIndex] = holder;
        const int editedIndex = editingAccountIndex;

        populateTable();
        updateSummaryCards();
        saveAccounts();
        updateInvestmentChart();
        selectAccountInTable(editedIndex);
        setFormMode(true);
        setRightRailVisible(true);
        showProjectionView(editedIndex);
        showLoader();
        appendAdminLog("Account updated: " + holder.name);

        QMessageBox::information(this, "Saved", "Account changes saved successfully.");
        return;
    }

    accounts.append(holder);

    populateTable();
    updateSummaryCards();
    saveAccounts();
    updateInvestmentChart();
    showLoader();
    clearForm();
    showAccountsView();
    appendAdminLog("Account created: " + holder.name);

    QMessageBox::information(this, "Success", "Account holder added successfully.");
}


// ─────────────────────────────────────────────────────────────────────────────
// on_btnDelete_clicked — deactivates or deletes selected row from QVector
// Owner: Keith
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_btnDelete_clicked()
{
    int row = ui->tableWidget->currentRow();
    int accountIndex = accountIndexForRow(row);

    if (accountIndex < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an account first.");
        return;
    }

    QMessageBox confirm(this);
    confirm.setIcon(QMessageBox::Question);
    confirm.setWindowTitle("Delete or Deactivate");
    confirm.setText("Choose how to handle the selected account.");
    QPushButton *deactivateButton = confirm.addButton("Deactivate", QMessageBox::AcceptRole);
    QPushButton *deleteButton = confirm.addButton("Delete", QMessageBox::DestructiveRole);
    confirm.addButton(QMessageBox::Cancel);
    confirm.exec();

    if (confirm.clickedButton() == deactivateButton) {
        accounts[accountIndex].status = "Inactive";
        populateTable();
        updateSummaryCards();
        saveAccounts();
        updateInvestmentChart();
        showLoader();
        clearForm();
        showAccountsView();
    } else if (confirm.clickedButton() == deleteButton) {
        accounts.removeAt(accountIndex);
        populateTable();
        updateSummaryCards();
        saveAccounts();
        updateInvestmentChart();
        showLoader();
        clearForm();
        showAccountsView();
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// on_btnClear_clicked — resets all input form fields
// Owner: Keith
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_btnClear_clicked()
{
    clearForm();
}


// ─────────────────────────────────────────────────────────────────────────────
// on_tableWidget_cellClicked — fills all form fields from the selected row
// Owner: Keith
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_tableWidget_cellClicked(int row, int column)
{
    Q_UNUSED(column);

    int accountIndex = accountIndexForRow(row);
    if (accountIndex < 0) return;

    const AccountHolder &h = accounts[accountIndex];
    editingAccountIndex = accountIndex;
    ui->inputCardNumber->setText(h.cardNumber);
    ui->inputName->setText(h.name);
    ui->inputUsername->setText(h.userName);
    ui->comboAccountType->setCurrentText(h.accountType);
    ui->comboStatus->setCurrentText(h.status);
    ui->inputSavings->setValue(h.savings);
    ui->spinInterest->setValue(fixedInterestRate(h.savings));
    setFormMode(true);
    setRightRailVisible(true);
    showProjectionView(accountIndex);
    scrollRightRailTo(ui->formPanel);
    appendAdminLog("Editing account: " + h.name);
}


// ─────────────────────────────────────────────────────────────────────────────
// on_searchBar_textChanged — filters table rows by account holder name
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_searchBar_textChanged(const QString &text)
{
    Q_UNUSED(text);
    applyFilters();
}


// ─────────────────────────────────────────────────────────────────────────────
// on_categoryFilter_currentIndexChanged — filters table rows by account type
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_categoryFilter_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    applyFilters();
}


// ─────────────────────────────────────────────────────────────────────────────
// on_sortCombo_currentIndexChanged — rebuilds table with selected sort mode
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_sortCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    applyFilters();
}


// ─────────────────────────────────────────────────────────────────────────────
// on_themeToggle_toggled — switches between light and dark dashboard themes
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_themeToggle_toggled(bool checked)
{
    applyTheme(checked);
}


// ─────────────────────────────────────────────────────────────────────────────
// Sidebar actions — lightweight navigation for the current single-window app
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_btnDashboard_clicked()
{
    setActiveNav(ui->btnDashboard);
    ui->screenTitle->setText("Banking Tracker Dashboard");
    ui->screenSubtitle->setText("Monitor account savings, account status, and administrative activity.");
    ui->searchBar->clear();
    ui->categoryFilter->setCurrentText("All");
    ui->sortCombo->setCurrentIndex(0);
    ui->tableWidget->setFocus();
    setRightRailVisible(false);
    showAccountsView();
    appendAdminLog("Dashboard opened");
}


void MainWindow::on_btnTransferMenu_clicked()
{
    setActiveNav(ui->btnTransferMenu);
    ui->screenTitle->setText("Transfer Workspace");
    ui->screenSubtitle->setText("Select an account record before recording transfer details.");
    ui->categoryFilter->setCurrentText("All");
    ui->tableWidget->setFocus();
    appendAdminLog("Transfer workspace opened");
}


void MainWindow::on_btnSavingsMenu_clicked()
{
    setActiveNav(ui->btnSavingsMenu);
    ui->screenTitle->setText("Savings Accounts");
    ui->screenSubtitle->setText("Filtered view for savings account records.");
    ui->categoryFilter->setCurrentText("Savings");
    applyFilters();
    ui->tableWidget->setFocus();
    appendAdminLog("Savings filter applied");
}


void MainWindow::on_btnCreateMenu_clicked()
{
    setActiveNav(ui->btnCreateMenu);
    ui->screenTitle->setText("Create Account");
    ui->screenSubtitle->setText("Enter customer identity and financial information.");
    clearForm();
    setRightRailVisible(true);
    showAccountsView();
    scrollRightRailTo(ui->formPanel);
    ui->inputName->setFocus();
    appendAdminLog("Create account form opened");
}


void MainWindow::on_btnTransactionsMenu_clicked()
{
    setActiveNav(ui->btnTransactionsMenu);
    showAdminLogsView();
    appendAdminLog("Transaction history merged into admin logs");
}


void MainWindow::on_btnReportsMenu_clicked()
{
    setActiveNav(ui->btnReportsMenu);
    showAccountsView();
    appendAdminLog("Summary reports removed; dashboard opened");
}


void MainWindow::on_btnAdminLogsMenu_clicked()
{
    setActiveNav(ui->btnAdminLogsMenu);
    showAdminLogsView();
    appendAdminLog("Admin logs opened");
}


void MainWindow::on_btnChangePassword_clicked()
{
    setActiveNav(ui->btnAdminLogsMenu);
    if (changeAdminPassword()) {
        showAdminLogsView();
        appendAdminLog("Admin password changed");
        QMessageBox::information(this, "Password Updated",
                                 "The admin password was updated successfully.");
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// applyFilters — applies search, category filter, and sort simultaneously
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::applyFilters()
{
    populateTable();
    showLoader();
}


// ─────────────────────────────────────────────────────────────────────────────
// updateSummaryCards — recomputes total accounts, savings, and average interest
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::updateSummaryCards()
{
    int    totalAccounts    = accounts.size();
    int    activeAccounts   = 0;
    double totalSavings     = 0.0;
    double interestTotal    = 0.0;

    for (const AccountHolder &h : accounts) {
        totalSavings     += h.savings;
        interestTotal    += fixedInterestRate(h.savings);
        if (h.status == "Active") {
            activeAccounts++;
        }
    }

    ui->lblTotalAccounts->setText(QString::number(totalAccounts));
    ui->lblTotalSavings->setText(formatCurrency(totalSavings));
    ui->lblTotalInvestments->setText(accounts.isEmpty()
                                         ? "0.00%"
                                         : QString::number(interestTotal / accounts.size(), 'f', 2) + "%");
    ui->lblActiveAccounts->setText(QString::number(activeAccounts));
}

// ─────────────────────────────────────────────────────────────────────────────
// populateTable — rebuilds account table from the source QVector
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::populateTable()
{
    const QString searchText = ui->searchBar->text().trimmed().toLower();
    const QString selectedType = ui->categoryFilter->currentText();
    const QString sortMode = ui->sortCombo->currentText();

    QVector<int> visibleAccounts;
    for (int i = 0; i < accounts.size(); ++i) {
        const AccountHolder &h = accounts[i];
        const QString searchable = QString("%1 %2 %3")
                                       .arg(h.cardNumber, h.name, h.userName)
                                       .toLower();

        const bool searchMatch = searchText.isEmpty() || searchable.contains(searchText);
        const bool typeMatch = selectedType.isEmpty() || selectedType == "All"
                               || h.accountType == selectedType;

        if (searchMatch && typeMatch) {
            visibleAccounts.append(i);
        }
    }

    std::sort(visibleAccounts.begin(), visibleAccounts.end(),
              [this, sortMode](int left, int right) {
                  const AccountHolder &a = accounts[left];
                  const AccountHolder &b = accounts[right];

                  if (sortMode == "Alphabetical") {
                      return QString::localeAwareCompare(a.name, b.name) < 0;
                  }
                  if (sortMode == "Savings: High to Low") {
                      return a.savings > b.savings;
                  }
                  if (sortMode == "Username") {
                      return QString::localeAwareCompare(a.userName, b.userName) < 0;
                  }
                  if (sortMode == "Status") {
                      return QString::localeAwareCompare(a.status, b.status) < 0;
                  }
                  return left > right;
              });

    ui->tableWidget->setRowCount(0);

    for (int row = 0; row < visibleAccounts.size(); ++row) {
        const int accountIndex = visibleAccounts[row];
        const AccountHolder &h = accounts[accountIndex];

        ui->tableWidget->insertRow(row);

        QStringList values = {
            h.cardNumber,
            h.name,
            h.userName,
            h.accountType,
            formatCurrency(h.savings),
            QString::number(fixedInterestRate(h.savings), 'f', 2) + "%",
            h.status
        };

        for (int col = 0; col < values.size(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(values[col]);
            item->setData(Qt::UserRole, accountIndex);
            item->setToolTip(values[col]);
            if (col >= 4 && col <= 5) {
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            } else if (col == 6) {
                item->setTextAlignment(Qt::AlignCenter);
            } else {
                item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                if (col == 1) {
                    QFont nameFont = item->font();
                    nameFont.setWeight(QFont::DemiBold);
                    item->setFont(nameFont);
                }
            }
            ui->tableWidget->setItem(row, col, item);
        }

        setRowColor(row);
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// setRowColor — applies color coding to the account status cell
// Owner: Aaron
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setRowColor(int row)
{
    QTableWidgetItem *statusItem = ui->tableWidget->item(row, 6);
    if (!statusItem) return;

    const QString status = statusItem->text();

    if (status == "Active") {
        statusItem->setBackground(QBrush(QColor("#DCEFEA")));
        statusItem->setForeground(QBrush(QColor("#146352")));
    } else if (status == "Inactive") {
        statusItem->setBackground(QBrush(QColor("#FFF0D9")));
        statusItem->setForeground(QBrush(QColor("#A05A00")));
    } else {
        statusItem->setBackground(QBrush(QColor("#FFE4E4")));
        statusItem->setForeground(QBrush(QColor("#B42318")));
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// computeStatus — returns a fallback status if the status combo has no value
// Owner: Aaron
// ─────────────────────────────────────────────────────────────────────────────
QString MainWindow::computeStatus(double savings)
{
    return savings > 0.0 ? "Active" : "Inactive";
}


// ─────────────────────────────────────────────────────────────────────────────
// clearForm — resets all input widgets to their default/empty state
// Owner: Keith
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::clearForm()
{
    editingAccountIndex = -1;
    ui->inputName->clear();
    ui->inputUsername->clear();
    ui->inputCardNumber->clear();
    ui->comboAccountType->setCurrentIndex(0);
    ui->comboStatus->setCurrentIndex(0);
    ui->inputSavings->setValue(0.0);
    updateAutomatedInterest();
    ui->tableWidget->clearSelection();
    setFormMode(false);
    updateInvestmentChart();
}


// ─────────────────────────────────────────────────────────────────────────────
// accountIndexForRow — maps a visible/sorted table row back to the QVector index
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
int MainWindow::accountIndexForRow(int row) const
{
    if (row < 0) return -1;

    QTableWidgetItem *item = ui->tableWidget->item(row, 0);
    if (!item) return -1;

    bool ok = false;
    int accountIndex = item->data(Qt::UserRole).toInt(&ok);
    if (!ok || accountIndex < 0 || accountIndex >= accounts.size()) {
        return -1;
    }

    return accountIndex;
}


// ─────────────────────────────────────────────────────────────────────────────
// applyTheme — keeps the default light palette and overlays a dark palette
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::applyTheme(bool darkMode)
{
    if (!darkMode) {
        ui->themeToggle->setText("Light Mode");
        setStyleSheet(lightThemeStyle);
        applyComboPopupTheme(false);
        return;
    }

    ui->themeToggle->setText("Dark Mode");
    setStyleSheet(lightThemeStyle + R"(

QMainWindow,
QWidget#centralwidget,
QWidget#contentArea {
    background: #101820;
    color: #E6EDF3;
}

QFrame#panel,
QFrame#tablePanel,
QFrame#formPanel,
QFrame#chartPanel,
QFrame#miniPanel,
QFrame#projectionPanel,
QFrame#mainLogsPanel,
QFrame#summaryCard1,
QFrame#summaryCard2,
QFrame#summaryCard3,
QFrame#summaryCard4 {
    background: #162330;
    border: 1px solid #2A3A48;
}

QLabel#screenTitle,
QLabel#lblTotalAccounts,
QLabel#lblTotalSavings,
QLabel#lblTotalInvestments,
QLabel#lblActiveAccounts {
    color: #E6EDF3;
}

QLabel#screenSubtitle,
QLabel#mutedLabel1,
QLabel#mutedLabel2,
QLabel#mutedLabel3,
QLabel#mutedLabel4,
QLabel#mutedLabel5,
QLabel#mutedLabel6,
QLabel#mutedLabel7,
QLabel#mutedLabel8,
QLabel#mutedLabel9,
QLabel#mutedLabel10,
QLabel#projectionSubtitle {
    color: #9AAAB8;
}

QLabel#cardTitle1,
QLabel#cardTitle2,
QLabel#cardTitle3,
QLabel#cardTitle4,
QLabel#panelTitle1,
QLabel#panelTitle2,
QLabel#panelTitle3,
QLabel#panelTitle4,
QLabel#projectionTitle {
    color: #D7E3EA;
}

QLineEdit,
QComboBox,
QSpinBox,
QDoubleSpinBox {
    background: #101820;
    border: 1px solid #34495A;
    color: #E6EDF3;
    selection-background-color: #2C937E;
}

QPushButton#btnClear {
    background: #243241;
    border: 1px solid #34495A;
    color: #D7E3EA;
}

QPushButton#btnDelete {
    background: #3A2025;
    border: 1px solid #7C3336;
    color: #FFB4AB;
}

QTableWidget {
    background: #162330;
    alternate-background-color: #1B2A38;
    border: 1px solid #2A3A48;
    color: #E6EDF3;
    gridline-color: #2A3A48;
    selection-background-color: #1D3C36;
    selection-color: #E6EDF3;
}

QComboBox QAbstractItemView,
QAbstractItemView {
    background: #162330;
    border: 1px solid #34495A;
    color: #E6EDF3;
    selection-background-color: #2E6FBF;
    selection-color: #FFFFFF;
    outline: 0;
}

QHeaderView::section {
    background: #203040;
    border-right: 1px solid #304253;
    color: #CAD6E0;
}

QCheckBox,
QListWidget {
    color: #CAD6E0;
}
)");
    applyComboPopupTheme(true);
}

void MainWindow::applyComboPopupTheme(bool darkMode)
{
    const QString popupStyle = darkMode
        ? "QAbstractItemView { background: #162330; color: #E6EDF3; "
          "selection-background-color: #2E6FBF; selection-color: #FFFFFF; "
          "border: 1px solid #34495A; outline: 0; }"
        : "QAbstractItemView { background: #FFFFFF; color: #162033; "
          "selection-background-color: #E8F0FE; selection-color: #162033; "
          "border: 1px solid #CAD5E2; outline: 0; }";

    const QList<QComboBox *> combos = findChildren<QComboBox *>();
    for (QComboBox *combo : combos) {
        combo->view()->setStyleSheet(popupStyle);
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// setActiveNav — updates sidebar active state for clickable menu items
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setActiveNav(QPushButton *activeButton)
{
    QList<QPushButton *> navButtons = {
        ui->btnDashboard,
        ui->btnCreateMenu,
        ui->btnAdminLogsMenu
    };

    for (QPushButton *button : navButtons) {
        const bool active = button == activeButton;
        button->setProperty("active", active);
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// appendAdminLog — records visible UI/navigation actions in the log panel
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::appendAdminLog(const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString("hh:mm AP");
    const QString line = QString("%1  %2").arg(time, message);
    ui->adminLogsList->insertItem(0, line);
    if (mainAdminLogsList) {
        mainAdminLogsList->insertItem(0, line);
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// scrollRightRailTo — brings right-side panels into view inside the scroll rail
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::scrollRightRailTo(QWidget *widget)
{
    if (!widget) return;

    ui->rightRailScroll->ensureWidgetVisible(widget, 0, 12);
}


// ─────────────────────────────────────────────────────────────────────────────
// setFormMode — labels the form clearly as create mode or edit mode
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setFormMode(bool editing)
{
    ui->panelTitle2->setText(editing ? "Edit Account" : "Create Account");
    ui->mutedLabel6->setText(editing
                                 ? "Update the selected customer record."
                                 : "Enter a new customer's identity and financial information.");
    ui->btnAdd->setText(editing ? "Save Changes" : "Create Account");
}


// ─────────────────────────────────────────────────────────────────────────────
// selectAccountInTable — restores table selection after rebuilding filtered rows
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::selectAccountInTable(int accountIndex)
{
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = ui->tableWidget->item(row, 0);
        if (item && item->data(Qt::UserRole).toInt() == accountIndex) {
            ui->tableWidget->selectRow(row);
            return;
        }
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QPoint globalPos = mouseEvent->globalPosition().toPoint();
#else
        const QPoint globalPos = mouseEvent->globalPos();
#endif
        QWidget *clicked = QApplication::widgetAt(globalPos);

        QWidget *focusCandidate = clicked;
        while (focusCandidate && focusCandidate != this) {
            if (qobject_cast<QLineEdit *>(focusCandidate)
                || qobject_cast<QComboBox *>(focusCandidate)
                || qobject_cast<QAbstractSpinBox *>(focusCandidate)) {
                focusCandidate->setFocus(Qt::MouseFocusReason);
                break;
            }
            focusCandidate = focusCandidate->parentWidget();
        }

        if (editingAccountIndex >= 0 && ui->formPanel->isVisible()) {
            const bool popupClick = clicked && clicked->window()
            && clicked->window()->windowFlags().testFlag(Qt::Popup);
            const bool tableClick = isInsideWidget(clicked, ui->tableWidget);
            const bool formClick = isInsideWidget(clicked, ui->formPanel);
            const bool projectionClick = isInsideWidget(clicked, projectionPanel);

            if (!popupClick && !tableClick && !formClick && !projectionClick) {
                collapseEditPanel();
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupModernUi()
{
    ui->btnTransferMenu->hide();
    ui->btnSavingsMenu->hide();
    ui->btnTransactionsMenu->hide();
    ui->btnReportsMenu->hide();
    ui->themeToggle->setText("Light Mode");
    ui->brandMark->setStyleSheet("background: #2E6FBF; border-radius: 8px; color: #FFFFFF; "
                                 "font-size: 18px; font-weight: 900;");
    ui->brandName->setStyleSheet("font-size: 18px; font-weight: 900;");
    ui->brandSubtitle->setStyleSheet("color: #A8B4C4; font-size: 11px; font-weight: 700;");
    ui->securityPanel->setStyleSheet("");
    ui->btnChangePassword->setStyleSheet("");

    sidebarToggleButton = new QPushButton(QString::fromUtf8("\xE2\x98\xB0"), ui->sidebar);
    sidebarToggleButton->setObjectName("btnSidebarToggle");
    sidebarToggleButton->setFixedSize(34, 34);
    sidebarToggleButton->setToolTip("Toggle navigation");
    sidebarToggleButton->setCursor(Qt::PointingHandCursor);
    ui->brandLayout->addWidget(sidebarToggleButton);
    connect(sidebarToggleButton, &QPushButton::clicked, this, &MainWindow::toggleSidebar);

    const QList<QPushButton *> actionButtons = findChildren<QPushButton *>();
    for (QPushButton *button : actionButtons) {
        button->setCursor(Qt::PointingHandCursor);
    }

    exitButton = new QPushButton("Exit", ui->sidebar);
    exitButton->setObjectName("btnExitProgram");
    exitButton->setCursor(Qt::PointingHandCursor);
    exitButton->setToolTip("Exit program");
    ui->sidebarLayout->insertWidget(ui->sidebarLayout->count() - 1, exitButton);
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::exitProgram);

    loadingBar = new QProgressBar(ui->contentArea);
    loadingBar->setObjectName("loadingBar");
    loadingBar->setRange(0, 0);
    loadingBar->setTextVisible(false);
    loadingBar->setFixedHeight(4);
    loadingBar->hide();
    ui->contentLayout->insertWidget(1, loadingBar);

    ui->cardTitle3->setText("Average Interest");
    ui->mutedLabel3->setText("Fixed by savings balance");
    ui->mutedLabel4->setText("Not suspended/inactive");

    ui->panelTitle3->setText("Forecast");
    ui->mutedLabel7->setText("Open an account to view the large projection.");
    ui->chartPanel->hide();
    ui->miniPanel->hide();
    ui->chartPanel->setMinimumHeight(250);
    ui->chartPanel->setMaximumHeight(300);
    ui->bar1->hide();
    ui->bar2->hide();
    ui->bar3->hide();
    ui->bar4->hide();
}

void MainWindow::setRightRailVisible(bool visible)
{
    ui->rightRailScroll->setVisible(visible);
    ui->mainWorkspaceLayout->setStretch(0, visible ? 1 : 2);
    ui->mainWorkspaceLayout->setStretch(1, visible ? 0 : 0);
}

void MainWindow::setupMainViews()
{
    projectionPanel = new QFrame(ui->tablePanel);
    projectionPanel->setObjectName("projectionPanel");
    auto *projectionLayout = new QVBoxLayout(projectionPanel);
    projectionLayout->setContentsMargins(18, 18, 18, 18);
    projectionLayout->setSpacing(14);

    auto *projectionHeader = new QHBoxLayout();
    auto *projectionText = new QVBoxLayout();
    projectionTitle = new QLabel("Savings Forecast", projectionPanel);
    projectionTitle->setObjectName("projectionTitle");
    projectionTitle->setStyleSheet("font-size: 20px; font-weight: 800;");
    projectionSubtitle = new QLabel("Select an account to view capital, projected value, and interest.", projectionPanel);
    projectionSubtitle->setObjectName("projectionSubtitle");
    projectionText->addWidget(projectionTitle);
    projectionText->addWidget(projectionSubtitle);
    projectionHeader->addLayout(projectionText, 1);
    backToAccountsButton = new QPushButton("Back to Accounts", projectionPanel);
    backToAccountsButton->setCursor(Qt::PointingHandCursor);
    projectionHeader->addWidget(backToAccountsButton);
    connect(backToAccountsButton, &QPushButton::clicked, this, &MainWindow::showAccountsView);
    projectionLayout->addLayout(projectionHeader);

    investmentChart = new CompoundInterestChart(projectionPanel);
    projectionLayout->addWidget(investmentChart, 1);

    projectionTable = new QTableWidget(projectionPanel);
    projectionTable->setColumnCount(4);
    projectionTable->setHorizontalHeaderLabels({"Term", "Capital", "Projected", "Interest"});
    projectionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    projectionTable->setSelectionMode(QAbstractItemView::NoSelection);
    projectionTable->verticalHeader()->setVisible(false);
    projectionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    projectionTable->setMinimumHeight(180);
    projectionLayout->addWidget(projectionTable);
    ui->tablePanelLayout->addWidget(projectionPanel);

    mainLogsPanel = new QFrame(ui->tablePanel);
    mainLogsPanel->setObjectName("mainLogsPanel");
    auto *logsLayout = new QVBoxLayout(mainLogsPanel);
    logsLayout->setContentsMargins(18, 18, 18, 18);
    logsLayout->setSpacing(12);
    auto *logsTitle = new QLabel("Admin Logs", mainLogsPanel);
    logsTitle->setObjectName("projectionTitle");
    logsTitle->setStyleSheet("font-size: 20px; font-weight: 800;");
    auto *logsSubtitle = new QLabel("All admin actions and file-system activity appear here.", mainLogsPanel);
    logsSubtitle->setObjectName("projectionSubtitle");
    mainAdminLogsList = new QListWidget(mainLogsPanel);
    mainAdminLogsList->setMinimumHeight(420);
    logsLayout->addWidget(logsTitle);
    logsLayout->addWidget(logsSubtitle);
    logsLayout->addWidget(mainAdminLogsList, 1);
    for (int i = 0; i < ui->adminLogsList->count(); ++i) {
        mainAdminLogsList->addItem(ui->adminLogsList->item(i)->text());
    }
    ui->tablePanelLayout->addWidget(mainLogsPanel);
}

void MainWindow::showAccountsView()
{
    ui->panelTitle1->setText("Account Directory");
    ui->mutedLabel5->setText("Search, filter, sort, create, deactivate, or review records.");
    ui->tableWidget->show();
    if (projectionPanel) {
        projectionPanel->hide();
    }
    if (mainLogsPanel) {
        mainLogsPanel->hide();
    }
}

void MainWindow::showProjectionView(int accountIndex)
{
    if (accountIndex < 0 || accountIndex >= accounts.size()) {
        return;
    }

    setActiveNav(ui->btnDashboard);
    ui->screenTitle->setText("Account Forecast");
    ui->screenSubtitle->setText("Fixed-rate savings projection for the selected account.");
    ui->panelTitle1->setText("Savings Forecast");
    ui->mutedLabel5->setText("Capital, projected value, and interest over 5, 10, 20, and 40 years.");
    ui->tableWidget->hide();
    if (mainLogsPanel) {
        mainLogsPanel->hide();
    }
    if (projectionPanel) {
        projectionPanel->show();
    }

    const AccountHolder &holder = accounts[accountIndex];
    const double rate = fixedInterestRate(holder.savings);
    projectionTitle->setText(holder.name + " Forecast");
    projectionSubtitle->setText(QString("%1 savings at %2% fixed annual interest")
                                    .arg(formatCurrency(holder.savings),
                                         QString::number(rate, 'f', 2)));
    projectionTable->setRowCount(0);
    const QVector<int> years = {5, 10, 20, 40};
    for (int row = 0; row < years.size(); ++row) {
        const int year = years[row];
        const double projected = projectedValue(holder.savings, rate, year);
        const double interest = projected - holder.savings;
        projectionTable->insertRow(row);
        const QStringList values = {
            QString::number(year) + " years",
            formatCurrency(holder.savings),
            formatCurrency(projected),
            formatCurrency(interest)
        };
        for (int col = 0; col < values.size(); ++col) {
            auto *item = new QTableWidgetItem(values[col]);
            item->setTextAlignment(col == 0 ? Qt::AlignLeft | Qt::AlignVCenter
                                            : Qt::AlignRight | Qt::AlignVCenter);
            projectionTable->setItem(row, col, item);
        }
    }
    updateInvestmentChart();
}

void MainWindow::showAdminLogsView()
{
    ui->screenTitle->setText("Admin Logs");
    ui->screenSubtitle->setText("Central log view for admin actions and saved-file activity.");
    ui->panelTitle1->setText("Admin Logs");
    ui->mutedLabel5->setText("Newest activity appears first.");
    setRightRailVisible(false);
    ui->tableWidget->hide();
    if (projectionPanel) {
        projectionPanel->hide();
    }
    if (mainLogsPanel) {
        mainLogsPanel->show();
        mainAdminLogsList->setFocus();
    }
}

void MainWindow::collapseEditPanel()
{
    if (editingAccountIndex < 0) {
        return;
    }

    clearForm();
    setRightRailVisible(false);
    appendAdminLog("Edit panel minimized");
}

void MainWindow::toggleSidebar()
{
    sidebarCollapsed = !sidebarCollapsed;
    const int width = sidebarCollapsed ? 76 : 252;

    auto *animation = new QVariantAnimation(this);
    animation->setDuration(180);
    animation->setStartValue(ui->sidebar->width());
    animation->setEndValue(width);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const int animatedWidth = value.toInt();
        ui->sidebar->setMinimumWidth(animatedWidth);
        ui->sidebar->setMaximumWidth(animatedWidth);
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    ui->brandName->setVisible(!sidebarCollapsed);
    ui->brandSubtitle->setVisible(!sidebarCollapsed);
    ui->securityPanel->setVisible(!sidebarCollapsed);
    sidebarToggleButton->setText(QString::fromUtf8("\xE2\x98\xB0"));
    sidebarToggleButton->setToolTip(sidebarCollapsed ? "Expand navigation"
                                                     : "Collapse navigation");

    ui->sidebarLayout->setContentsMargins(sidebarCollapsed ? 12 : 18,
                                          22,
                                          sidebarCollapsed ? 12 : 18,
                                          18);

    const QList<QPair<QPushButton *, QString>> labels = {
        {ui->btnDashboard, "Dashboard"},
        {ui->btnCreateMenu, "Create Account"},
        {ui->btnAdminLogsMenu, "Admin Logs"},
        {exitButton, "Exit"}
    };

    for (const auto &entry : labels) {
        QPushButton *button = entry.first;
        if (!button) {
            continue;
        }
        const QString &label = entry.second;
        button->setToolTip(label);
        const QString collapsedLabel = label == "Create Account" ? "+"
                                      : label == "Admin Logs" ? "Logs"
                                      : label == "Exit" ? "Exit"
                                                        : "Home";
        button->setText(sidebarCollapsed ? collapsedLabel : label);
        button->setProperty("collapsed", sidebarCollapsed);
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
}

void MainWindow::exitProgram()
{
    appendAdminLog("Exit requested");
    close();
}

void MainWindow::showLoader()
{
    if (!loadingBar) {
        return;
    }

    loadingBar->show();
    QTimer::singleShot(360, loadingBar, [bar = loadingBar]() {
        if (bar) {
            bar->hide();
        }
    });
}

void MainWindow::loadAccounts()
{
    QFile file(dataFilePath());
    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        appendAdminLog("Unable to read saved account data");
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        appendAdminLog("Saved account data is invalid");
        return;
    }

    accounts.clear();
    const QJsonArray savedAccounts = document.array();
    for (const QJsonValue &value : savedAccounts) {
        const QJsonObject object = value.toObject();
        AccountHolder holder;
        holder.cardNumber = object.value("cardNumber").toString();
        holder.name = object.value("name").toString();
        holder.userName = object.value("userName").toString();
        holder.accountType = object.value("accountType").toString("Savings");
        holder.savings = object.value("savings").toDouble();
        holder.investments = object.value("investments").toDouble();
        holder.monthsInvested = object.value("monthsInvested").toInt();
        holder.interestRate = fixedInterestRate(holder.savings);
        holder.status = object.value("status").toString(computeStatus(holder.savings));

        if (!holder.name.isEmpty() && !holder.cardNumber.isEmpty()) {
            accounts.append(holder);
        }
    }

    appendAdminLog(QString("Loaded %1 saved accounts").arg(accounts.size()));
}

void MainWindow::saveAccounts() const
{
    QJsonArray savedAccounts;
    for (const AccountHolder &holder : accounts) {
        QJsonObject object;
        object["cardNumber"] = holder.cardNumber;
        object["name"] = holder.name;
        object["userName"] = holder.userName;
        object["accountType"] = holder.accountType;
        object["savings"] = holder.savings;
        object["investments"] = holder.investments;
        object["monthsInvested"] = holder.monthsInvested;
        object["interestRate"] = holder.interestRate;
        object["status"] = holder.status;
        savedAccounts.append(object);
    }

    QSaveFile file(dataFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    file.write(QJsonDocument(savedAccounts).toJson(QJsonDocument::Indented));
    file.commit();
}

QString MainWindow::dataFilePath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        appDataPath = QDir::homePath() + "/BankTracker";
    }

    QDir().mkpath(appDataPath);
    return appDataPath + "/accounts.json";
}

QString MainWindow::buildLightThemeStyle() const
{
    return R"(
QMainWindow,
QWidget#centralwidget {
    background: #F4F6F8;
    color: #1F2937;
    font-family: "Inter", "SF Pro Display", "Segoe UI";
}

QWidget#contentArea {
    background: #F4F6F8;
}

QFrame#sidebar {
    background: #172033;
    border: none;
}

QFrame#sidebar QLabel {
    color: #EEF2F7;
}

QFrame#sidebar QPushButton {
    background: transparent;
    border: none;
    border-radius: 8px;
    color: #C7D2E1;
    font-size: 14px;
    font-weight: 650;
    padding: 10px 12px;
    text-align: left;
}

QFrame#sidebar QPushButton[collapsed="true"] {
    text-align: center;
    padding: 10px 0;
}

QFrame#sidebar QPushButton:hover {
    background: #22314C;
    color: #FFFFFF;
}

QFrame#sidebar QPushButton[active="true"] {
    background: #2E6FBF;
    color: #FFFFFF;
}

QPushButton#btnSidebarToggle {
    background: #22314C;
    border: 1px solid #33445F;
    color: #FFFFFF;
    padding: 0;
    text-align: center;
}

QFrame#securityPanel {
    background: #202D45;
    border: 1px solid #33445F;
    border-radius: 8px;
}

QFrame#panel,
QFrame#tablePanel,
QFrame#formPanel,
QFrame#chartPanel,
QFrame#miniPanel,
QFrame#projectionPanel,
QFrame#mainLogsPanel {
    background: #FFFFFF;
    border: 1px solid #DCE3EC;
    border-radius: 8px;
}

QFrame#summaryCard1,
QFrame#summaryCard2,
QFrame#summaryCard3,
QFrame#summaryCard4 {
    background: #FFFFFF;
    border: 1px solid #DCE3EC;
    border-radius: 8px;
}

QLabel#screenTitle {
    color: #162033;
    font-size: 25px;
    font-weight: 800;
}

QLabel#screenSubtitle,
QLabel#mutedLabel1,
QLabel#mutedLabel2,
QLabel#mutedLabel3,
QLabel#mutedLabel4,
QLabel#mutedLabel5,
QLabel#mutedLabel6,
QLabel#mutedLabel7,
QLabel#mutedLabel8,
QLabel#mutedLabel9,
QLabel#mutedLabel10 {
    color: #607086;
    font-size: 12px;
    font-weight: 600;
}

QLabel#cardTitle1,
QLabel#cardTitle2,
QLabel#cardTitle3,
QLabel#cardTitle4,
QLabel#panelTitle1,
QLabel#panelTitle2,
QLabel#panelTitle3,
QLabel#panelTitle4,
QLabel#projectionTitle {
    color: #2D3A4D;
    font-size: 13px;
    font-weight: 750;
}

QLabel#lblTotalAccounts,
QLabel#lblTotalSavings,
QLabel#lblTotalInvestments,
QLabel#lblActiveAccounts {
    color: #162033;
    font-size: 24px;
    font-weight: 800;
}

QLineEdit,
QComboBox,
QSpinBox,
QDoubleSpinBox {
    background: #FFFFFF;
    border: 1px solid #CAD5E2;
    border-radius: 7px;
    color: #1F2937;
    font-weight: 650;
    min-height: 34px;
    padding: 6px 12px;
    selection-background-color: #2E6FBF;
}

QLineEdit:hover,
QComboBox:hover,
QSpinBox:hover,
QDoubleSpinBox:hover {
    border-color: #9FB1C6;
}

QLineEdit:focus,
QComboBox:focus,
QSpinBox:focus,
QDoubleSpinBox:focus {
    border: 1px solid #2E6FBF;
}

QComboBox::drop-down {
    border: none;
    width: 30px;
}

QComboBox QAbstractItemView {
    background: #FFFFFF;
    border: 1px solid #CAD5E2;
    selection-background-color: #E8F0FE;
    selection-color: #162033;
    padding: 4px;
}

QSpinBox::up-button,
QSpinBox::down-button,
QDoubleSpinBox::up-button,
QDoubleSpinBox::down-button {
    border: none;
    width: 0px;
}

QPushButton {
    border-radius: 7px;
    font-weight: 700;
    min-height: 34px;
    padding: 8px 14px;
}

QPushButton#btnAdd,
QPushButton#btnChangePassword {
    background: #2E6FBF;
    border: 1px solid #2E6FBF;
    color: #FFFFFF;
}

QPushButton#btnAdd:hover,
QPushButton#btnChangePassword:hover {
    background: #245DA3;
}

QPushButton#btnClear {
    background: #EEF2F7;
    border: 1px solid #CAD5E2;
    color: #2D3A4D;
}

QPushButton#btnClear:hover {
    background: #E2E8F0;
}

QPushButton#btnDelete {
    background: #FFF1F2;
    border: 1px solid #FDA4AF;
    color: #B42318;
}

QPushButton#btnDelete:hover {
    background: #FFE4E6;
}

QTableWidget {
    background: #FFFFFF;
    alternate-background-color: #F8FAFC;
    border: 1px solid #DCE3EC;
    border-radius: 8px;
    color: #1F2937;
    gridline-color: #E5EAF0;
    selection-background-color: #E8F0FE;
    selection-color: #162033;
}

QHeaderView::section {
    background: #EEF2F7;
    border: none;
    border-right: 1px solid #DCE3EC;
    color: #42526A;
    font-size: 12px;
    font-weight: 800;
    min-height: 34px;
    padding: 8px;
}

QTableWidget::item {
    padding: 6px 10px;
}

QCheckBox#themeToggle {
    color: #42526A;
    font-weight: 700;
    spacing: 8px;
}

QCheckBox#themeToggle::indicator {
    width: 34px;
    height: 18px;
    border-radius: 9px;
    background: #CBD5E1;
}

QCheckBox#themeToggle::indicator:checked {
    background: #2E6FBF;
}

QProgressBar#loadingBar {
    background: transparent;
    border: none;
}

QProgressBar#loadingBar::chunk {
    background: #2E6FBF;
    border-radius: 2px;
}

QScrollArea#rightRailScroll,
QScrollArea#rightRailScroll > QWidget > QWidget {
    background: transparent;
    border: none;
}

QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 2px;
}

QScrollBar::handle:vertical {
    background: #94A3B8;
    border-radius: 4px;
    min-height: 28px;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0px;
}

QListWidget {
    background: transparent;
    border: none;
    color: #42526A;
}

QListWidget::item {
    border-radius: 7px;
    padding: 8px;
}

QListWidget::item:selected {
    background: #E8F0FE;
    color: #162033;
}
)";
}

QString MainWindow::formatCurrency(double value) const
{
    return PesoSymbol + QLocale(QLocale::English).toString(value, 'f', 2);
}

double MainWindow::fixedInterestRate(double savings) const
{
    if (savings >= 1000000.0) {
        return 5.00;
    }
    if (savings >= 250000.0) {
        return 4.25;
    }
    if (savings >= 50000.0) {
        return 3.50;
    }
    if (savings > 0.0) {
        return 2.25;
    }
    return 0.0;
}

double MainWindow::projectedValue(double principal, double annualRatePercent, int years) const
{
    if (principal <= 0.0 || years <= 0) {
        return principal;
    }
    return principal * std::pow(1.0 + (annualRatePercent / 100.0), years);
}

void MainWindow::updateAutomatedInterest()
{
    const double rate = fixedInterestRate(ui->inputSavings->value());
    ui->spinInterest->blockSignals(true);
    ui->spinInterest->setValue(rate);
    ui->spinInterest->blockSignals(false);
}

void MainWindow::updateInvestmentChart()
{
    if (!investmentChart) {
        return;
    }

    double principal = 0.0;
    double weightedRate = 0.0;

    if (editingAccountIndex >= 0 && editingAccountIndex < accounts.size()) {
        const AccountHolder &holder = accounts[editingAccountIndex];
        principal = holder.savings;
        weightedRate = fixedInterestRate(holder.savings);
    } else if (ui->rightRailScroll->isVisible()) {
        principal = ui->inputSavings->value();
        weightedRate = ui->spinInterest->value();
    }

    if (principal <= 0.0) {
        double portfolioPrincipal = 0.0;
        double rateSum = 0.0;

        for (const AccountHolder &holder : accounts) {
            const double accountPrincipal = holder.savings;
            if (accountPrincipal <= 0.0) {
                continue;
            }

            const double rate = fixedInterestRate(holder.savings);
            portfolioPrincipal += accountPrincipal;
            rateSum += accountPrincipal * rate;
        }

        principal = portfolioPrincipal;
        weightedRate = portfolioPrincipal > 0.0 ? rateSum / portfolioPrincipal : 0.0;
    }

    investmentChart->setProjection(principal, weightedRate);
}

void MainWindow::ensureAdminCredentials()
{
    QSettings settings("BankTracker", "TraackingSystem");
    if (settings.contains("auth/passwordHash") && settings.contains("auth/salt")) {
        return;
    }

    QByteArray salt;
    salt.resize(16);
    for (char &byte : salt) {
        byte = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }

    settings.setValue("auth/salt", QString::fromLatin1(salt.toHex()));
    settings.setValue("auth/passwordHash",
                      QString::fromLatin1(adminPasswordHash("admin123", salt).toHex()));
    settings.setValue("auth/firstRun", true);
}

QByteArray MainWindow::adminPasswordHash(const QString &password, const QByteArray &salt) const
{
    return QCryptographicHash::hash(salt + password.toUtf8(),
                                    QCryptographicHash::Sha256);
}

bool MainWindow::verifyAdminPassword(const QString &password) const
{
    QSettings settings("BankTracker", "TraackingSystem");
    const QByteArray salt = QByteArray::fromHex(settings.value("auth/salt").toByteArray());
    const QByteArray expectedHash =
        QByteArray::fromHex(settings.value("auth/passwordHash").toByteArray());
    return adminPasswordHash(password, salt) == expectedHash;
}

bool MainWindow::authenticateAdmin()
{
    ensureAdminCredentials();
    QSettings settings("BankTracker", "TraackingSystem");

    for (int attempt = 0; attempt < 3; ++attempt) {
        QDialog dialog(this);
        dialog.setObjectName("adminLoginDialog");
        dialog.setWindowTitle("Admin Login");
        dialog.setModal(true);
        dialog.setMinimumWidth(360);

        auto *layout = new QVBoxLayout(&dialog);
        auto *title = new QLabel("Admin Login", &dialog);
        title->setObjectName("authTitle");
        title->setStyleSheet("font-size: 20px; font-weight: 800; color: #E5E5E5;");
        auto *subtitle = new QLabel(settings.value("auth/firstRun", false).toBool()
                                        ? "First run: admin / admin123"
                                        : "Enter admin credentials", &dialog);
        subtitle->setStyleSheet("color: #607086; font-weight: 600;");

        auto *form = new QFormLayout();
        auto *username = new QLineEdit(&dialog);
        auto *password = new QLineEdit(&dialog);
        username->setText("admin");
        password->setEchoMode(QLineEdit::Password);
        password->setPlaceholderText("Password");
        form->addRow("Username", username);
        form->addRow("Password", password);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                             &dialog);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        layout->addWidget(title);
        layout->addWidget(subtitle);
        layout->addLayout(form);
        layout->addWidget(buttons);

        if (dialog.exec() != QDialog::Accepted) {
            return false;
        }

        if (username->text().trimmed() == "admin" && verifyAdminPassword(password->text())) {
            settings.setValue("auth/firstRun", false);
            ui->adminNameLabel->setText("Admin User");
            ui->adminRoleLabel->setText("Authenticated session");
            appendAdminLog("Admin login successful");
            return true;
        }

        QMessageBox::warning(this, "Login Failed",
                             QString("Invalid credentials. %1 attempt(s) remaining.")
                                 .arg(2 - attempt));
    }

    QMessageBox::critical(this, "Access Locked",
                          "Too many failed login attempts. Please restart and try again.");
    return false;
}

bool MainWindow::changeAdminPassword()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Change Admin Password");
    dialog.setModal(true);
    dialog.setMinimumWidth(390);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *currentPassword = new QLineEdit(&dialog);
    auto *newPassword = new QLineEdit(&dialog);
    auto *confirmPassword = new QLineEdit(&dialog);
    currentPassword->setEchoMode(QLineEdit::Password);
    newPassword->setEchoMode(QLineEdit::Password);
    confirmPassword->setEchoMode(QLineEdit::Password);
    form->addRow("Current", currentPassword);
    form->addRow("New", newPassword);
    form->addRow("Confirm", confirmPassword);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addLayout(form);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    if (!verifyAdminPassword(currentPassword->text())) {
        QMessageBox::warning(this, "Password Error", "The current password is incorrect.");
        return false;
    }

    if (newPassword->text().size() < 8) {
        QMessageBox::warning(this, "Password Error",
                             "Use at least 8 characters for the new password.");
        return false;
    }

    if (newPassword->text() != confirmPassword->text()) {
        QMessageBox::warning(this, "Password Error", "The new passwords do not match.");
        return false;
    }

    QByteArray salt;
    salt.resize(16);
    for (char &byte : salt) {
        byte = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }

    QSettings settings("BankTracker", "TraackingSystem");
    settings.setValue("auth/salt", QString::fromLatin1(salt.toHex()));
    settings.setValue("auth/passwordHash",
                      QString::fromLatin1(adminPasswordHash(newPassword->text(), salt).toHex()));
    settings.setValue("auth/firstRun", false);
    return true;
}
