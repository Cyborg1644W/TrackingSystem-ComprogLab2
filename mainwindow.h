#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>

class QPushButton;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ─────────────────────────────────────────────
// AccountHolder struct — Reindel
// Stores all 7 required fields per account holder
// ─────────────────────────────────────────────
struct AccountHolder {
    QString cardNumber;
    QString name;
    QString userName;
    QString accountType;   // "Savings" | "Time Deposit" | "Stocks"
    double  savings;
    double  investments;
    int     monthsInvested;
    double  interestRate;
    QString status;        // "Active" | "Inactive" | "Suspended"
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
    void on_sortCombo_currentIndexChanged(int index);
    void on_themeToggle_toggled(bool checked);
    void on_tableWidget_cellClicked(int row, int column);
    void on_btnDashboard_clicked();
    void on_btnTransferMenu_clicked();
    void on_btnSavingsMenu_clicked();
    void on_btnCreateMenu_clicked();
    void on_btnTransactionsMenu_clicked();
    void on_btnReportsMenu_clicked();
    void on_btnAdminLogsMenu_clicked();
    void on_btnChangePassword_clicked();

private:
    Ui::MainWindow *ui;
    QString lightThemeStyle;
    int editingAccountIndex = -1;

    // ── Data (Reindel) ─────────────────────────
    QVector<AccountHolder> accounts;  // Master list — no parallel arrays

    // ── Helper functions ───────────────────────
    void updateSummaryCards();        // Reindel — recompute totals after add/delete
    void applyFilters();              // Reindel — apply search + category together
    void populateTable();             // Reindel — rebuild table from QVector
    void clearForm();                 // Keith   — reset all input fields
    void setRowColor(int row);        // Aaron   — color-code status column
    QString computeStatus(double savings); // Aaron — fallback account status
    int accountIndexForRow(int row) const;
    void applyTheme(bool darkMode);
    void setActiveNav(QPushButton *activeButton);
    void appendAdminLog(const QString &message);
    void scrollRightRailTo(QWidget *widget);
    void setFormMode(bool editing);
    void selectAccountInTable(int accountIndex);
};

#endif // MAINWINDOW_.
