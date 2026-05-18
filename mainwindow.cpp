#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QColor>
#include <QBrush>
#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QDateTime>
#include <QHeaderView>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QTableWidgetItem>

#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor — wires all signals & slots, sets up table headers and filters
// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    lightThemeStyle = styleSheet();
    setWindowTitle("BankTracker — Banking & Investment Tracker");

    QStringList headers = {"Card Number", "Name", "Username", "Account Type",
                           "Savings (₱)", "Investments (₱)", "Account Age",
                           "Interest Rate", "Status"};
    ui->tableWidget->setColumnCount(headers.size());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    // ── Category filter options (Aaron) ───────────────────────────────────
    ui->categoryFilter->addItems({"All", "Savings", "Time Deposit", "UITF", "Stocks"});

    // ── Account type combo (Aaron) ─────────────────────────────────────────
    ui->comboAccountType->addItems({"Savings", "Time Deposit", "UITF", "Stocks"});
    ui->comboStatus->addItems({"Active", "Inactive", "Suspended"});
    ui->sortCombo->addItems({"Sort: Latest", "Alphabetical", "Savings: High to Low",
                             "Username", "Investments: High to Low", "Account Age"});

    ui->contentLayout->setStretch(2, 1);
    ui->mainWorkspaceLayout->setStretch(0, 1);
    ui->mainWorkspaceLayout->setStretch(1, 0);
    setActiveNav(ui->btnDashboard);

    ui->inputCardNumber->setInputMask("9999 9999 9999 9999;_");
    ui->inputSavings->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->inputInvestments->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->spinMonths->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->spinInterest->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->inputSavings->setGroupSeparatorShown(true);
    ui->inputInvestments->setGroupSeparatorShown(true);
    ui->spinMonths->setGroupSeparatorShown(true);
    ui->inputSavings->setSingleStep(1000.0);
    ui->inputInvestments->setSingleStep(1000.0);
    ui->spinMonths->setSingleStep(1);
    ui->spinInterest->setSingleStep(0.25);
    ui->inputSavings->setAlignment(Qt::AlignRight);
    ui->inputInvestments->setAlignment(Qt::AlignRight);
    ui->spinMonths->setAlignment(Qt::AlignRight);
    ui->spinInterest->setAlignment(Qt::AlignRight);
    setFormMode(false);

    // ── Initial summary cards (Reindel) ───────────────────────────────────
    updateSummaryCards();
    populateTable();
}

MainWindow::~MainWindow()
{
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
    double  investments = ui->inputInvestments->value();
    int     months      = ui->spinMonths->value();
    double  interest    = ui->spinInterest->value();
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
        selectAccountInTable(editedIndex);
        setFormMode(true);
        appendAdminLog("Account updated: " + holder.name);

        QMessageBox::information(this, "Saved", "Account changes saved successfully.");
        return;
    }

    accounts.append(holder);

    populateTable();
    updateSummaryCards();
    clearForm();
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
        clearForm();
    } else if (confirm.clickedButton() == deleteButton) {
        accounts.removeAt(accountIndex);
        populateTable();
        updateSummaryCards();
        clearForm();
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
    ui->inputInvestments->setValue(h.investments);
    ui->spinMonths->setValue(h.monthsInvested);
    ui->spinInterest->setValue(h.interestRate);
    setFormMode(true);
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
    ui->screenSubtitle->setText("Monitor account savings, investments, status, and administrative activity.");
    ui->searchBar->clear();
    ui->categoryFilter->setCurrentText("All");
    ui->sortCombo->setCurrentIndex(0);
    ui->tableWidget->setFocus();
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
    scrollRightRailTo(ui->formPanel);
    ui->inputName->setFocus();
    appendAdminLog("Create account form opened");
}


void MainWindow::on_btnTransactionsMenu_clicked()
{
    setActiveNav(ui->btnTransactionsMenu);
    ui->screenTitle->setText("Transaction History");
    ui->screenSubtitle->setText("Recent activity is shown in the admin log panel until full history storage is added.");
    scrollRightRailTo(ui->miniPanel);
    appendAdminLog("Transaction history panel opened");
}


void MainWindow::on_btnReportsMenu_clicked()
{
    setActiveNav(ui->btnReportsMenu);
    ui->screenTitle->setText("Summary Reports");
    ui->screenSubtitle->setText("Savings, investment, and status summaries are visible in the dashboard cards.");
    scrollRightRailTo(ui->chartPanel);
    appendAdminLog("Summary report preview opened");
}


void MainWindow::on_btnAdminLogsMenu_clicked()
{
    setActiveNav(ui->btnAdminLogsMenu);
    ui->screenTitle->setText("Admin Logs");
    ui->screenSubtitle->setText("Administrative actions and system placeholders are listed in the log panel.");
    scrollRightRailTo(ui->miniPanel);
    ui->adminLogsList->setFocus();
    appendAdminLog("Admin logs opened");
}


void MainWindow::on_btnChangePassword_clicked()
{
    setActiveNav(ui->btnAdminLogsMenu);
    appendAdminLog("Password change dialog requested");
    QMessageBox::information(this, "Change Password",
                             "Password-change UI is reserved for the admin login module.");
}


// ─────────────────────────────────────────────────────────────────────────────
// applyFilters — applies search, category filter, and sort simultaneously
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::applyFilters()
{
    populateTable();
}


// ─────────────────────────────────────────────────────────────────────────────
// updateSummaryCards — recomputes total accounts, savings, and investments
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::updateSummaryCards()
{
    int    totalAccounts    = accounts.size();
    int    activeAccounts   = 0;
    double totalSavings     = 0.0;
    double totalInvestments = 0.0;

    for (const AccountHolder &h : accounts) {
        totalSavings     += h.savings;
        totalInvestments += h.investments;
        if (h.status == "Active") {
            activeAccounts++;
        }
    }

    ui->lblTotalAccounts->setText(QString::number(totalAccounts));
    ui->lblTotalSavings->setText("₱" + QString::number(totalSavings, 'f', 2));
    ui->lblTotalInvestments->setText("₱" + QString::number(totalInvestments, 'f', 2));
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
        if (sortMode == "Investments: High to Low") {
            return a.investments > b.investments;
        }
        if (sortMode == "Account Age") {
            return a.monthsInvested > b.monthsInvested;
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
            "₱" + QString::number(h.savings, 'f', 2),
            "₱" + QString::number(h.investments, 'f', 2),
            QString::number(h.monthsInvested) + " months",
            QString::number(h.interestRate, 'f', 2) + "%",
            h.status
        };

        for (int col = 0; col < values.size(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(values[col]);
            item->setData(Qt::UserRole, accountIndex);
            if (col >= 4 && col <= 7) {
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            } else if (col == 8) {
                item->setTextAlignment(Qt::AlignCenter);
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
    QTableWidgetItem *statusItem = ui->tableWidget->item(row, 8);
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
    ui->inputInvestments->setValue(0.0);
    ui->spinMonths->setValue(0);
    ui->spinInterest->setValue(0.0);
    ui->tableWidget->clearSelection();
    setFormMode(false);
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
        setStyleSheet(lightThemeStyle);
        return;
    }

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
QLabel#mutedLabel10 {
    color: #9AAAB8;
}

QLabel#cardTitle1,
QLabel#cardTitle2,
QLabel#cardTitle3,
QLabel#cardTitle4,
QLabel#panelTitle1,
QLabel#panelTitle2,
QLabel#panelTitle3,
QLabel#panelTitle4 {
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
}


// ─────────────────────────────────────────────────────────────────────────────
// setActiveNav — updates sidebar active state for clickable menu items
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setActiveNav(QPushButton *activeButton)
{
    QList<QPushButton *> navButtons = {
        ui->btnDashboard,
        ui->btnTransferMenu,
        ui->btnSavingsMenu,
        ui->btnCreateMenu,
        ui->btnTransactionsMenu,
        ui->btnReportsMenu,
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
    ui->adminLogsList->insertItem(0, QString("%1  %2").arg(time, message));
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
