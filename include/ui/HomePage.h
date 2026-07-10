#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class ThemeManager;

class HomePage final : public QWidget {
    Q_OBJECT

public:
    explicit HomePage(ThemeManager& themeManager, QWidget* parent = nullptr);

private slots:
    void applyTheme();

private:
    void buildUi();
    void updateWordmarkFont();
    QString navButtonStylesheet() const;

    void resizeEvent(QResizeEvent* event) override;

    ThemeManager& themeManager_;
    QLabel* neuroLabel_ = nullptr;
    QLabel* tendrilsLabel_ = nullptr;
    QPushButton* roboticArmButton_ = nullptr;
    QPushButton* eegToTextButton_ = nullptr;
    QPushButton* educationButton_ = nullptr;
};
