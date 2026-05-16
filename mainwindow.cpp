#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QColor>
#include <QBrush>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor — wires all signals & slots, sets up table headers and filters
// Owner: Keith (signals) + Aaron (UI setup) + Reindel (filter init)
// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("BankTracker — Banking & Investment Tracker");

    // ── Table headers (Aaron) ──────────────────────────────────────────────
    QStringList headers = {"Name", "Account Type", "Savings (₱)",
                           "Investments (₱)", "Months", "Interest Rate", "Status"};
    ui->tableWidget->setColumnCount(headers.size());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    // ── Category filter options (Aaron) ───────────────────────────────────
    ui->categoryFilter->addItem("All");
    ui->categoryFilter->addItem("Savings");
    ui->categoryFilter->addItem("Time Deposit");
    ui->categoryFilter->addItem("UITF");
    ui->categoryFilter->addItem("Stocks");

    // ── Account type combo (Aaron) ─────────────────────────────────────────
    ui->comboAccountType->addItem("Savings");
    ui->comboAccountType->addItem("Time Deposit");
    ui->comboAccountType->addItem("UITF");
    ui->comboAccountType->addItem("Stocks");

    // ── Signal connections (Keith) ─────────────────────────────────────────
    connect(ui->btnAdd,    &QPushButton::clicked, this, &MainWindow::on_btnAdd_clicked);
    connect(ui->btnDelete, &QPushButton::clicked, this, &MainWindow::on_btnDelete_clicked);
    connect(ui->btnClear,  &QPushButton::clicked, this, &MainWindow::on_btnClear_clicked);

    connect(ui->searchBar,      &QLineEdit::textChanged,
            this, &MainWindow::on_searchBar_textChanged);
    connect(ui->categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_categoryFilter_currentIndexChanged);

    connect(ui->tableWidget, &QTableWidget::cellClicked,
            this, &MainWindow::on_tableWidget_cellClicked);

    // ── Initial summary cards (Reindel) ───────────────────────────────────
    updateSummaryCards();
}

MainWindow::~MainWindow()
{
    delete ui;
}


// ─────────────────────────────────────────────────────────────────────────────
// on_btnAdd_clicked — reads all 7 form fields, validates, adds to QVector + table
// Owner: Keith
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_btnAdd_clicked()
{
    // Read all fields
    QString name        = ui->inputName->text().trimmed();
    QString accountType = ui->comboAccountType->currentText();
    double  savings     = ui->inputSavings->value();
    double  investments = ui->inputInvestments->value();
    int     months      = ui->spinMonths->value();
    double  interest    = ui->spinInterest->value();
    QString status      = computeStatus(savings);

    // Validate: name must not be empty
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Name field cannot be empty.");
        return;
    }

    // Build struct and add to QVector
    AccountHolder holder;
    holder.name           = name;
    holder.accountType    = accountType;
    holder.savings        = savings;
    holder.investments    = investments;
    holder.monthsInvested = months;
    holder.interestRate   = interest;
    holder.status         = status;
    accounts.append(holder);

    // Add row to table
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(holder.name));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(holder.accountType));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(holder.savings, 'f', 2)));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(holder.investments, 'f', 2)));
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(holder.monthsInvested)));
    ui->tableWidget->setItem(row, 5, new QTableWidgetItem(QString::number(holder.interestRate, 'f', 2) + "%"));
    ui->tableWidget->setItem(row, 6, new QTableWidgetItem(holder.status));

    // Apply status color (Aaron)
    setRowColor(row);

    // Update summary cards and clear form
    updateSummaryCards();
    clearForm();

    QMessageBox::information(this, "Success", "Account holder added successfully.");
}


// ─────────────────────────────────────────────────────────────────────────────
// on_btnDelete_clicked — confirms then deletes selected row from table + QVector
// Owner: Keith
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::on_btnDelete_clicked()
{
    int row = ui->tableWidget->currentRow();

    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a row to delete.");
        return;
    }

    // Confirm with Yes/No dialog
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Delete",
        "Are you sure you want to delete this account holder?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        accounts.removeAt(row);
        ui->tableWidget->removeRow(row);
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

    if (row < 0 || row >= accounts.size()) return;

    AccountHolder &h = accounts[row];
    ui->inputName->setText(h.name);
    ui->comboAccountType->setCurrentText(h.accountType);
    ui->inputSavings->setValue(h.savings);
    ui->inputInvestments->setValue(h.investments);
    ui->spinMonths->setValue(h.monthsInvested);
    ui->spinInterest->setValue(h.interestRate);
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
// applyFilters — applies name search AND category filter simultaneously
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::applyFilters()
{
    QString searchText   = ui->searchBar->text().trimmed().toLower();
    QString selectedType = ui->categoryFilter->currentText();

    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        QString rowName = ui->tableWidget->item(row, 0)->text().toLower();
        QString rowType = ui->tableWidget->item(row, 1)->text();

        bool nameMatch = rowName.contains(searchText);
        bool typeMatch = (selectedType == "All") || (rowType == selectedType);

        ui->tableWidget->setRowHidden(row, !(nameMatch && typeMatch));
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// updateSummaryCards — recomputes total accounts, savings, and investments
// Owner: Reindel
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::updateSummaryCards()
{
    int    totalAccounts    = accounts.size();
    double totalSavings     = 0.0;
    double totalInvestments = 0.0;

    for (const AccountHolder &h : accounts) {
        totalSavings     += h.savings;
        totalInvestments += h.investments;
    }

    ui->lblTotalAccounts->setText(QString::number(totalAccounts));
    ui->lblTotalSavings->setText("₱" + QString::number(totalSavings, 'f', 2));
    ui->lblTotalInvestments->setText("₱" + QString::number(totalInvestments, 'f', 2));
}


// ─────────────────────────────────────────────────────────────────────────────
// setRowColor — applies background color to status cell based on savings
// Owner: Aaron
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setRowColor(int row)
{
    QString status = ui->tableWidget->item(row, 6)->text();
    QColor color;

    if (status == "High")      color = QColor("#B7EFC5"); // Green
    else if (status == "Mid")  color = QColor("#FFE08A"); // Yellow
    else                       color = QColor("#FFADAD"); // Red

    for (int col = 0; col < ui->tableWidget->columnCount(); col++) {
        if (ui->tableWidget->item(row, col))
            ui->tableWidget->item(row, col)->setBackground(QBrush(color));
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// computeStatus — returns "High", "Mid", or "Low" based on savings amount
// Owner: Aaron
// ─────────────────────────────────────────────────────────────────────────────
QString MainWindow::computeStatus(double savings)
{
    if (savings >= 50000.0) return "High";
    if (savings >= 10000.0) return "Mid";
    return "Low";
}


// ─────────────────────────────────────────────────────────────────────────────
// clearForm — resets all input widgets to their default/empty state
// Owner: Keith
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::clearForm()
{
    ui->inputName->clear();
    ui->comboAccountType->setCurrentIndex(0);
    ui->inputSavings->setValue(0.0);
    ui->inputInvestments->setValue(0.0);
    ui->spinMonths->setValue(0);
    ui->spinInterest->setValue(0.0);
}
