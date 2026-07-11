#include "data/AppConfig.hpp"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QtEndian>

#include "data/AppPaths.hpp"

#include <functional>

namespace {

// Fixed presentation order for the motion controls. Regions inherit this order
// from the first motion that references them, which keeps the panel layout and
// the brain region list stable regardless of JSON key ordering.
const QStringList kMotionOrder = {
    QStringLiteral("hand_open"),
    QStringLiteral("hand_close"),
    QStringLiteral("arm_raise"),
    QStringLiteral("arm_lower"),
    QStringLiteral("base_rotate_left"),
    QStringLiteral("base_rotate_right"),
    QStringLiteral("elbow_raise"),
    QStringLiteral("elbow_lower"),
    QStringLiteral("arm_reset"),
};

bool readJson(const QString& resourcePath, QJsonObject* out, QString* error) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = QStringLiteral("Missing config resource: %1").arg(resourcePath);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error != nullptr) {
            *error = QStringLiteral("Malformed JSON in %1: %2")
                         .arg(resourcePath, parseError.errorString());
        }
        return false;
    }

    *out = document.object();
    return true;
}

} // namespace

bool AppConfig::load(QString* error) {
    if (!loadBrainRegions(error) || !loadRoArmCommands(error) || !loadLearningCards(error)) {
        return false;
    }
    resolveModelIndices();
    return true;
}

// Qt's Quick3D RuntimeLoader imports meshes in depth-first glTF node order but
// drops the node names, so region highlighting works on indices. This reads the
// GLB's JSON chunk and maps each region's configured mesh names (prefix match,
// covering ".l"/".r" hemisphere suffixes) to positions in that traversal.
void AppConfig::resolveModelIndices() {
    QFile file(AppPaths::brainModelFile());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QByteArray blob = file.readAll();
    if (blob.size() < 20 || !blob.startsWith("glTF")) {
        return;
    }
    const quint32 jsonLength = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar*>(blob.constData() + 12));

    const int jsonStart = 20;
    const QJsonDocument document = QJsonDocument::fromJson(blob.mid(jsonStart, jsonLength));
    if (!document.isObject()) {
        return;
    }
    const QJsonObject root = document.object();
    const QJsonArray nodes = root.value(QStringLiteral("nodes")).toArray();
    const QJsonArray scenes = root.value(QStringLiteral("scenes")).toArray();
    const int sceneIndex = root.value(QStringLiteral("scene")).toInt(0);
    if (sceneIndex >= scenes.size()) {
        return;
    }

    QStringList meshNodeNames;
    std::function<void(int)> walk = [&](int nodeIndex) {
        const QJsonObject node = nodes.at(nodeIndex).toObject();
        if (node.contains(QStringLiteral("mesh"))) {
            meshNodeNames.append(node.value(QStringLiteral("name")).toString());
        }
        const QJsonArray children = node.value(QStringLiteral("children")).toArray();
        for (const QJsonValue& child : children) {
            walk(child.toInt());
        }
    };

    const QJsonArray sceneNodes =
        scenes.at(sceneIndex).toObject().value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue& rootNode : sceneNodes) {
        walk(rootNode.toInt());
    }

    for (BrainRegion& region : regions_) {
        for (int i = 0; i < meshNodeNames.size(); ++i) {
            for (const QString& needle : region.meshNames) {
                if (!needle.isEmpty() && meshNodeNames.at(i).startsWith(needle)) {
                    region.modelIndices.append(i);
                    break;
                }
            }
        }
    }

    // Aggregate the POSITION accessor bounds so the scene can frame the camera.
    // glTF requires min/max on POSITION accessors, which survive Draco decoding.
    const QJsonArray accessors = root.value(QStringLiteral("accessors")).toArray();
    const QJsonArray meshes = root.value(QStringLiteral("meshes")).toArray();
    QVector3D lo(1e9F, 1e9F, 1e9F);
    QVector3D hi(-1e9F, -1e9F, -1e9F);
    bool any = false;

    for (const QJsonValue& meshValue : meshes) {
        const QJsonArray primitives = meshValue.toObject().value(QStringLiteral("primitives")).toArray();
        for (const QJsonValue& primValue : primitives) {
            const QJsonObject attributes =
                primValue.toObject().value(QStringLiteral("attributes")).toObject();
            if (!attributes.contains(QStringLiteral("POSITION"))) {
                continue;
            }
            const int accessorIndex = attributes.value(QStringLiteral("POSITION")).toInt();
            if (accessorIndex < 0 || accessorIndex >= accessors.size()) {
                continue;
            }
            const QJsonObject accessor = accessors.at(accessorIndex).toObject();
            const QJsonArray mn = accessor.value(QStringLiteral("min")).toArray();
            const QJsonArray mx = accessor.value(QStringLiteral("max")).toArray();
            if (mn.size() != 3 || mx.size() != 3) {
                continue;
            }
            lo.setX(qMin(lo.x(), static_cast<float>(mn.at(0).toDouble())));
            lo.setY(qMin(lo.y(), static_cast<float>(mn.at(1).toDouble())));
            lo.setZ(qMin(lo.z(), static_cast<float>(mn.at(2).toDouble())));
            hi.setX(qMax(hi.x(), static_cast<float>(mx.at(0).toDouble())));
            hi.setY(qMax(hi.y(), static_cast<float>(mx.at(1).toDouble())));
            hi.setZ(qMax(hi.z(), static_cast<float>(mx.at(2).toDouble())));
            any = true;
        }
    }

    if (any) {
        modelMin_ = lo;
        modelMax_ = hi;
    }
}

bool AppConfig::loadBrainRegions(QString* error) {
    QJsonObject root;
    if (!readJson(QStringLiteral(":/config/brain_regions.json"), &root, error)) {
        return false;
    }

    const QJsonObject regionsObject = root.value(QStringLiteral("regions")).toObject();
    const QJsonObject motionsObject = root.value(QStringLiteral("motions")).toObject();

    QHash<QString, BrainRegion> regionsById;
    for (auto it = regionsObject.begin(); it != regionsObject.end(); ++it) {
        const QJsonObject value = it.value().toObject();
        BrainRegion region;
        region.id = it.key();
        region.label = value.value(QStringLiteral("label")).toString();
        region.shortLabel = value.value(QStringLiteral("short_label")).toString();
        region.description = value.value(QStringLiteral("description")).toString();
        region.color = QColor(value.value(QStringLiteral("color")).toString());

        const QJsonArray meshes =
            value.value(QStringLiteral("highlight")).toObject().value(QStringLiteral("mesh_names")).toArray();
        for (const QJsonValue& mesh : meshes) {
            region.meshNames.append(mesh.toString());
        }
        regionsById.insert(region.id, region);
    }

    QHash<QString, int> groupIndexByRegion;
    for (const QString& motionId : kMotionOrder) {
        if (!motionsObject.contains(motionId)) {
            continue;
        }
        const QJsonObject value = motionsObject.value(motionId).toObject();

        MotionCommand command;
        command.id = motionId;
        command.regionId = value.value(QStringLiteral("region")).toString();
        command.label = value.value(QStringLiteral("label")).toString();
        command.shortLabel = value.value(QStringLiteral("short_label")).toString();

        const auto regionIt = regionsById.constFind(command.regionId);
        if (regionIt == regionsById.constEnd()) {
            continue;
        }
        command.group = regionIt->shortLabel;

        // Record region ordering as motions reference them for the first time.
        if (!groupIndexByRegion.contains(command.regionId)) {
            regions_.append(*regionIt);
            motionGroups_.append(MotionGroup{command.group, {}});
            groupIndexByRegion.insert(command.regionId, motionGroups_.size() - 1);
        }
        motionGroups_[groupIndexByRegion.value(command.regionId)].commands.append(command);
    }

    if (regions_.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("No brain regions were loaded");
        }
        return false;
    }
    return true;
}

bool AppConfig::loadRoArmCommands(QString* error) {
    QJsonObject root;
    if (!readJson(QStringLiteral(":/config/roarm_commands.json"), &root, error)) {
        return false;
    }

    defaultArmHost_ = root.value(QStringLiteral("default_ip")).toString();
    const QJsonObject commands = root.value(QStringLiteral("commands")).toObject();

    for (MotionGroup& group : motionGroups_) {
        for (MotionCommand& command : group.commands) {
            const QJsonObject spec = commands.value(command.id).toObject();
            command.oneShot = spec.value(QStringLiteral("one_shot")).toBool(false);
            command.startPayload = spec.value(QStringLiteral("start")).toObject();
            command.stopPayload = spec.value(QStringLiteral("stop")).toObject();
        }
    }
    return true;
}

bool AppConfig::loadLearningCards(QString* error) {
    QJsonObject root;
    if (!readJson(QStringLiteral(":/config/education.json"), &root, error)) {
        return false;
    }

    const QJsonArray carousel = root.value(QStringLiteral("carousel")).toArray();
    for (const QJsonValue& value : carousel) {
        const QJsonObject card = value.toObject();
        learningCards_.append(LearningCard{
            card.value(QStringLiteral("category")).toString(),
            card.value(QStringLiteral("title")).toString(),
            card.value(QStringLiteral("body")).toString(),
        });
    }
    return true;
}

const BrainRegion* AppConfig::region(const QString& id) const {
    for (const BrainRegion& region : regions_) {
        if (region.id == id) {
            return &region;
        }
    }
    return nullptr;
}

const MotionCommand* AppConfig::motion(const QString& id) const {
    for (const MotionGroup& group : motionGroups_) {
        for (const MotionCommand& command : group.commands) {
            if (command.id == id) {
                return &command;
            }
        }
    }
    return nullptr;
}

QVector<LearningCard> AppConfig::cardsForRegion(const QString& regionId) const {
    Q_UNUSED(regionId);
    return learningCards_;
}
