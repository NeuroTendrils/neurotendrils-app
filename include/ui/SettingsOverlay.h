#pragma once

#include "theme/ColorPalette.h"

#include <QWidget>

class QButtonGroup;
class QKeyEvent;
class QLabel;
class QPushButton;
class QStackedWidget;
class ThemeManager;

class SettingsOverlay final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsOverlay(ThemeManager& themeManager, QWidget* parent = nullptr);

    void openPanel();
    void closePanel();
    bool isOpen() const;

signals:
    void opened();
    void closed();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void applyTheme();
    void syncThemeSelection(Theme theme);
    void showAppearancePage();
    void showInfoPage();

private:
    void buildUi();
    void layoutPanel();
    QString panelStylesheet() const;

    ThemeManager& themeManager_;
    QWidget* backdrop_ = nullptr;
    QWidget* panel_ = nullptr;
    QWidget* sidebar_ = nullptr;
    QWidget* contentArea_ = nullptr;
    QStackedWidget* contentStack_ = nullptr;
    QLabel* appearanceNav_ = nullptr;
    QLabel* infoNav_ = nullptr;
    QPushButton* lightThemeCard_ = nullptr;
    QPushButton* darkThemeCard_ = nullptr;
    QPushButton* autoThemeCard_ = nullptr;
    QButtonGroup* themeGroup_ = nullptr;
    bool open_ = false;
};
