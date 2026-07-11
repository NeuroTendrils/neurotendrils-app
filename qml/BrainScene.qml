import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.AssetUtils

// Interactive cortical atlas. The brain model is loaded at runtime and every
// mesh gets a shared neutral material; the active region's meshes swap to a
// glowing accent material. Colors are driven from C++ so the scene tracks the
// app theme. Names, colors and mesh lists all come from brain_regions.json.
Item {
    id: root

    property url modelSource
    property color baseColor: "#c7ccd8"
    property color glowColor: "#6f96ff"
    property color backgroundColor: "#0e1220"
    // Indices into the imported mesh list (depth-first glTF order); resolved on
    // the C++ side because RuntimeLoader drops node names.
    property var highlightIndices: []
    // Model bounding box supplied by C++ (RuntimeLoader reports zero bounds for
    // this Draco-decoded asset, so we frame the camera from these instead).
    property vector3d modelCenter: Qt.vector3d(0, 0, 0)
    property real modelSize: 0
    property bool autoRotate: true
    property bool ready: false

    // Loaded Model nodes, discovered once the asset finishes importing.
    property var _models: []

    View3D {
        id: view
        anchors.fill: parent
        camera: camera

        environment: SceneEnvironment {
            clearColor: root.backgroundColor
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        Node {
            id: originNode

            PerspectiveCamera {
                id: camera
                z: 420
                fieldOfView: 45
                clipNear: 1
                clipFar: 10000
            }

            // Slow idle rotation that pauses while the user is dragging.
            NumberAnimation on eulerRotation.y {
                running: root.autoRotate && root.ready
                from: originNode.eulerRotation.y
                to: originNode.eulerRotation.y + 360
                duration: 44000
                loops: Animation.Infinite
            }
        }

        DirectionalLight {
            eulerRotation.x: -35
            eulerRotation.y: -35
            brightness: 1.1
        }
        DirectionalLight {
            eulerRotation.x: -20
            eulerRotation.y: 150
            brightness: 0.55
        }
        DirectionalLight {
            eulerRotation.x: 80
            brightness: 0.35
        }

        PrincipledMaterial {
            id: baseMaterial
            baseColor: root.baseColor
            roughness: 0.62
            metalness: 0.0
        }

        PrincipledMaterial {
            id: glowMaterial
            baseColor: root.glowColor
            roughness: 0.35
            metalness: 0.0
            emissiveFactor: Qt.vector3d(root.glowColor.r * 0.55,
                                        root.glowColor.g * 0.55,
                                        root.glowColor.b * 0.55)
            SequentialAnimation on emissiveFactor {
                running: root.ready
                loops: Animation.Infinite
                Vector3dAnimation {
                    to: Qt.vector3d(root.glowColor.r * 0.95,
                                    root.glowColor.g * 0.95,
                                    root.glowColor.b * 0.95)
                    duration: 900
                    easing.type: Easing.InOutSine
                }
                Vector3dAnimation {
                    to: Qt.vector3d(root.glowColor.r * 0.4,
                                    root.glowColor.g * 0.4,
                                    root.glowColor.b * 0.4)
                    duration: 900
                    easing.type: Easing.InOutSine
                }
            }
        }

        RuntimeLoader {
            id: loader
            source: root.modelSource

            onStatusChanged: {
                if (status === RuntimeLoader.Success) {
                    root._collectModels()
                    root._normalizeToView()
                    root._applyHighlight()
                    root.ready = true
                } else if (status === RuntimeLoader.Error) {
                    console.warn("Brain model failed to load:", errorString)
                }
            }
        }
    }

    OrbitCameraController {
        anchors.fill: parent
        origin: originNode
        camera: camera
    }

    onHighlightIndicesChanged: _applyHighlight()
    onModelSizeChanged: if (root.ready) _normalizeToView()

    // Scale and center the imported model so it fills the viewport regardless of
    // the source model's native units. Uses the C++-supplied bounding box.
    function _normalizeToView() {
        if (modelSize <= 0)
            return
        var target = 300
        var s = target / modelSize
        loader.scale = Qt.vector3d(s, s, s)
        loader.position = Qt.vector3d(-modelCenter.x * s, -modelCenter.y * s, -modelCenter.z * s)
        originNode.eulerRotation = Qt.vector3d(-8, -28, 0)
    }

    function _collectModels() {
        var found = []
        _walk(loader, found)
        _models = found
    }

    function _walk(node, out) {
        if (!node || !node.children)
            return
        for (var i = 0; i < node.children.length; ++i) {
            var child = node.children[i]
            if (child.materials !== undefined)
                out.push(child)
            _walk(child, out)
        }
    }

    function _applyHighlight() {
        for (var i = 0; i < _models.length; ++i) {
            var lit = highlightIndices.indexOf(i) !== -1
            _models[i].materials = lit ? [glowMaterial] : [baseMaterial]
        }
    }
}
