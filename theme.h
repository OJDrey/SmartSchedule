#ifndef THEME_H
#define THEME_H

enum class Theme { System, Light, Dark };

namespace ThemeManager {
Theme saved();
void save(Theme theme);
void apply(Theme theme);
}

#endif // THEME_H