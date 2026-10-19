#include "mainwindow.h"
#include "databasemanager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Ініціалізація бази даних SQLite
    if (!DatabaseManager::instance().connectToDatabase("schedule.db")) {
        qDebug() << "Помилка відкриття бази даних. Програма завершується.";
        return -1;
    }

    MainWindow w;
    w.setWindowTitle("SmartSchedule - Автоматизація розкладу занять");
    w.resize(1080, 720);
    w.show();

    return a.exec();
}