#pragma once

#include <QUrl>
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
    void onQuickStatusChanged();

private:
    void applySceneProperties();

    ThemeManager& themeManager_;
    QQuickWidget* quickWidget_ = nullptr;

    bool sceneReady_ = false;
    QUrl modelSource_{QStringLiteral("qrc:/models/brain.glb")};
    QVector3D modelCenter_;
    float modelSize_ = 0.181F;
    bool hasBounds_ = false;
    QVector<int> highlightIndices_;
};
