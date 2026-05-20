#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>
#include <QByteArray>

class CompoundInterestChart;
class QEvent;
class QLabel;
class QListWidget;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


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
    bool authenticateAdmin();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

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
    bool sidebarCollapsed = false;
    QPushButton *sidebarToggleButton = nullptr;
    QPushButton *exitButton = nullptr;
    QPushButton *backToAccountsButton = nullptr;
    QProgressBar *loadingBar = nullptr;
    CompoundInterestChart *investmentChart = nullptr;
    QWidget *projectionPanel = nullptr;
    QWidget *mainLogsPanel = nullptr;
    QLabel *projectionTitle = nullptr;
    QLabel *projectionSubtitle = nullptr;
    QTableWidget *projectionTable = nullptr;
    QListWidget *mainAdminLogsList = nullptr;

    QVector<AccountHolder> accounts;

    void updateSummaryCards();
    void applyFilters();
    void populateTable();
    void clearForm();
    void setRowColor(int row);
    QString computeStatus(double savings);
    int accountIndexForRow(int row) const;
    void applyTheme(bool darkMode);
    void setActiveNav(QPushButton *activeButton);
    void appendAdminLog(const QString &message);
    void scrollRightRailTo(QWidget *widget);
    void setFormMode(bool editing);
    void selectAccountInTable(int accountIndex);
    void setupModernUi();
    void setRightRailVisible(bool visible);
    void collapseEditPanel();
    void toggleSidebar();
    void exitProgram();
    void showLoader();
    void loadAccounts();
    void saveAccounts() const;
    QString dataFilePath() const;
    QString buildLightThemeStyle() const;
    QString formatCurrency(double value) const;
    double fixedInterestRate(double savings) const;
    double projectedValue(double principal, double annualRatePercent, int years) const;
    void updateAutomatedInterest();
    void updateInvestmentChart();
    void setupMainViews();
    void showAccountsView();
    void showProjectionView(int accountIndex);
    void showAdminLogsView();
    void applyComboPopupTheme(bool darkMode);
    void ensureAdminCredentials();
    QByteArray adminPasswordHash(const QString &password, const QByteArray &salt) const;
    bool verifyAdminPassword(const QString &password) const;
    bool changeAdminPassword();
};

#endif // MAINWINDOW_.
