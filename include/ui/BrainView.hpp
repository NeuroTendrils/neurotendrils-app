#pragma once

#include <QVector>
#include <QVector3D>
#include <QWidget>

class QQuickWidget;
class ThemeManager;

// Hosts the Quick3D brain scene inside the widget UI and exposes a small,
// widget-friendly API: pick a highlighted region and follow the app theme.
class BrainView : public QWidget {
    Q_OBJECT

public:
    explicit BrainView(ThemeManager& themeManager, QWidget* parent = nullptr);

    void setModelBounds(const QVector3D& minimum, const QVector3D& maximum);
    void setHighlightIndices(const QVector<int>& modelIndices);
    void clearHighlight();

private slots:
    void applyTheme();

private:
    ThemeManager& themeManager_;
    QQuickWidget* quickWidget_ = nullptr;
};
