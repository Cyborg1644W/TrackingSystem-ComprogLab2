#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ─────────────────────────────────────────────
// AccountHolder struct — Reindel
// Stores all 7 required fields per account holder
// ─────────────────────────────────────────────
struct AccountHolder {
    QString name;
    QString accountType;   // "Savings" | "Time Deposit" | "UITF" | "Stocks"
    double  savings;
    double  investments;
    int     monthsInvested;
    double  interestRate;
    QString status;        // "High" | "Mid" | "Low"
};


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnAdd_clicked();
    void on_btnDelete_clicked();
    void on_btnClear_clicked();
    void on_searchBar_textChanged(const QString &text);
    void on_categoryFilter_currentIndexChanged(int index);
    void on_tableWidget_cellClicked(int row, int column);

private:
    Ui::MainWindow *ui;

    // ── Data (Reindel) ─────────────────────────
    QVector<AccountHolder> accounts;  // Master list — no parallel arrays

    // ── Helper functions ───────────────────────
    void updateSummaryCards();        // Reindel — recompute totals after add/delete
    void applyFilters();              // Reindel — apply search + category together
    void populateTable();             // Reindel — rebuild table from QVector
    void clearForm();                 // Keith   — reset all input fields
    void setRowColor(int row);        // Aaron   — color-code status column
    QString computeStatus(double savings); // Aaron — returns "High"/"Mid"/"Low"
};

#endif // MAINWINDOW_H
