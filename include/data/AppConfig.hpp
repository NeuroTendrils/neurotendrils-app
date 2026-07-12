#pragma once

#include <QColor>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>

// Domain model for the Education experience. Everything is loaded once from the
// bundled JSON configs (brain regions, motion-to-region mapping, RoArm command
// payloads, and the learning cards) so the UI layers stay declarative.

struct BrainRegion {
    QString id;
    QString label;
    QString shortLabel;
    QString description;
    QColor color;
    QStringList meshNames;
    // Indices into the runtime-loaded model's mesh list (depth-first glTF node
    // order). Qt's RuntimeLoader does not expose node names, so the brain scene
    // highlights meshes by index instead.
    QVector<int> modelIndices;
    // Preferred-hemisphere AABB center in model space (for floating labels).
    QVector3D anchorLocal;
    bool hasAnchor = false;
};

struct MotionCommand {
    QString id;
    QString regionId;
    QString label;
    QString shortLabel;
    QString group;
    bool oneShot = false;
    QJsonObject startPayload;
    QJsonObject stopPayload;
};

struct MotionGroup {
    QString title;
    QVector<MotionCommand> commands;
};

struct LearningCard {
    QString category;
    QString title;
    QString body;
};

class AppConfig {
public:
    // Loads from the embedded resource bundle. Returns false and fills `error`
    // if any config is missing or malformed.
    bool load(QString* error = nullptr);

    QString defaultArmHost() const { return defaultArmHost_; }

    const QVector<BrainRegion>& regions() const { return regions_; }
    const BrainRegion* region(const QString& id) const;

    const QVector<MotionGroup>& motionGroups() const { return motionGroups_; }
    const MotionCommand* motion(const QString& id) const;

    const QVector<LearningCard>& learningCards() const { return learningCards_; }

    // World-space bounding box of the brain model (computed from mesh data,
    // because Draco-decompressed accessors lose their min/max and RuntimeLoader
    // reports zero bounds). Used by the scene to frame the camera.
    QVector3D modelMin() const { return modelMin_; }
    QVector3D modelMax() const { return modelMax_; }

    // Learning cards whose category matches the region's teaching topic, falling
    // back to the full deck when nothing is tagged for it.
    QVector<LearningCard> cardsForRegion(const QString& regionId) const;

private:
    bool loadBrainRegions(QString* error);
    bool loadRoArmCommands(QString* error);
    bool loadLearningCards(QString* error);
    void resolveModelIndices();

    QString defaultArmHost_;
    QVector<BrainRegion> regions_;
    QVector<MotionGroup> motionGroups_;
    QVector<LearningCard> learningCards_;
    QVector3D modelMin_;
    QVector3D modelMax_;
};
