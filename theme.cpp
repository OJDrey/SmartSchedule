#include "theme.h"
#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

namespace {
struct SystemDefaults {
    QString style;
    QPalette palette;
};

const SystemDefaults &systemDefaults()
{
    static const SystemDefaults defaults{QApplication::style()->objectName(), QApplication::palette()};
    return defaults;
}

QPalette makePalette(bool dark)
{
    const QColor window = dark ? QColor(45, 45, 48) : QColor(240, 240, 240);
    const QColor base = dark ? QColor(30, 30, 30) : QColor(255, 255, 255);
    const QColor button = dark ? QColor(55, 55, 58) : QColor(240, 240, 240);
    const QColor text = dark ? QColor(235, 235, 235) : QColor(0, 0, 0);
    const QColor disabled = QColor(128, 128, 128);

    QPalette p;
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, window);
    p.setColor(QPalette::ToolTipBase, base);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, QColor(25, 118, 210));
    p.setColor(QPalette::Highlight, QColor(25, 118, 210));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::PlaceholderText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Text, disabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    return p;
}
}

Theme ThemeManager::saved()
{
    QSettings settings("SmartSchedule", "SmartSchedule");
    const int value = settings.value("theme", static_cast<int>(Theme::System)).toInt();
    return value >= 0 && value <= 2 ? static_cast<Theme>(value) : Theme::System;
}

void ThemeManager::save(Theme theme)
{
    QSettings settings("SmartSchedule", "SmartSchedule");
    settings.setValue("theme", static_cast<int>(theme));
}

void ThemeManager::apply(Theme theme)
{
    const SystemDefaults &defaults = systemDefaults();

    if (theme == Theme::System) {
        QApplication::setStyle(QStyleFactory::create(defaults.style));
        QApplication::setPalette(defaults.palette);
        return;
    }

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication::setPalette(makePalette(theme == Theme::Dark));
}