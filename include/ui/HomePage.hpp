#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class ThemeManager;

class HomePage final : public QWidget {
    Q_OBJECT

public:
    explicit HomePage(ThemeManager& themeManager, QWidget* parent = nullptr);

signals:
    void educationRequested();
    void roboticArmRequested();
    void eegToTextRequested();

private slots:
    void applyTheme();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildUi();
    void updateWordmarkFont();
    QString navButtonStylesheet() const;

    ThemeManager& themeManager_;
    QLabel* neuroLabel_ = nullptr;
    QLabel* tendrilsLabel_ = nullptr;
    QPushButton* roboticArmButton_ = nullptr;
    QPushButton* eegToTextButton_ = nullptr;
    QPushButton* educationButton_ = nullptr;
};
