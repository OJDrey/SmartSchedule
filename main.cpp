#include "mainwindow.h"
#include "databasemanager.h"
#include "theme.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("SmartSchedule");
    ThemeManager::apply(ThemeManager::saved());

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    if (!DatabaseManager::instance().connectToDatabase(dataDir + "/schedule.db")) {
        QMessageBox::critical(nullptr, "Помилка", "Не вдалося відкрити базу даних. Програма завершується.");
        return -1;
    }

    MainWindow w;
    w.setWindowTitle("SmartSchedule - Автоматизація розкладу занять");
    w.resize(1080, 720);
    w.show();

    return a.exec();
}