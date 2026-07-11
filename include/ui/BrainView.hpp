#pragma once

#include <QUrl>
#include <QVector>
#include <QVector3D>
#include <QWidget>

class QLabel;
class QQuickWidget;
class ThemeManager;

// Hosts the Quick3D brain scene inside the widget UI and exposes a small,
// widget-friendly API: pick a highlighted region and follow the app theme.
//
// The QML/3D scene is loaded lazily on first show. Creating a QQuickWidget
// while Education is still hidden in a QStackedWidget often leaves Quick3D
// without a usable surface, so the brain never appears.
class BrainView : public QWidget {
    Q_OBJECT

public:
    explicit BrainView(ThemeManager& themeManager, QWidget* parent = nullptr);

    void setModelBounds(const QVector3D& minimum, const QVector3D& maximum);
    void setHighlightIndices(const QVector<int>& modelIndices);
    void clearHighlight();

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void applyTheme();
    void onQuickStatusChanged();

private:
    void ensureSceneLoaded();
    void applySceneProperties();

    ThemeManager& themeManager_;
    QQuickWidget* quickWidget_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    bool sceneRequested_ = false;
    bool sceneReady_ = false;
    QUrl modelSource_;
    QVector3D modelCenter_{0.0F, 1.619F, -0.009F};
    float modelSize_ = 0.181F;
    bool hasBounds_ = true;
    QVector<int> highlightIndices_;
};
