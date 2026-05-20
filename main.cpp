#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("BankTracker");
    QApplication::setApplicationName("TraackingSystem");
    MainWindow w;
    if (!w.authenticateAdmin()) {
        return 0;
    }
    w.show();
    return a.exec();
}