#pragma once

#include <QWidget>

class QLabel;
class ThemeManager;

// Placeholder for features that are not ready yet.
class WipPage final : public QWidget {
    Q_OBJECT

public:
    WipPage(ThemeManager& themeManager, const QString& featureName, QWidget* parent = nullptr);

    void setFeatureName(const QString& featureName);

private slots:
    void applyTheme();

private:
    ThemeManager& themeManager_;
    QLabel* titleLabel_ = nullptr;
    QLabel* bodyLabel_ = nullptr;
    QString featureName_;
};
