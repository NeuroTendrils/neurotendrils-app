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

    property url modelSource: "qrc:/models/brain.glb"
    property color baseColor: "#b5aea0"
    property color glowColor: "#6f96ff"
    property color backgroundColor: "#ffffff"
    // Indices into the imported mesh list (depth-first glTF order); resolved on
    // the C++ side because RuntimeLoader drops node names.
    property var highlightIndices: []
    // Model bounding box supplied by C++ (RuntimeLoader reports zero bounds for
    // this Draco-decoded asset, so we frame the camera from these instead).
    property vector3d modelCenter: Qt.vector3d(0.0, 1.619, -0.009)
    property real modelSize: 0.181
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

            // Idle spin must stop while the user is dragging — otherwise it
            // fights OrbitCameraController and the brain feels stuck/wrong.
            NumberAnimation on eulerRotation.y {
                running: root.autoRotate && root.ready && !orbit.inputsNeedProcessing
                from: originNode.eulerRotation.y
                to: originNode.eulerRotation.y + 360
                duration: 44000
                loops: Animation.Infinite
            }
        }

        // Flat, illustrative lighting — one soft key and a weak fill so the
        // cortex reads as a diagram, not a photoreal render.
        DirectionalLight {
            eulerRotation.x: -40
            eulerRotation.y: -25
            brightness: 0.85
            ambientColor: Qt.rgba(0.55, 0.55, 0.58, 1)
        }
        DirectionalLight {
            eulerRotation.x: 20
            eulerRotation.y: 140
            brightness: 0.25
        }

        PrincipledMaterial {
            id: baseMaterial
            baseColor: root.baseColor
            roughness: 1.0
            metalness: 0.0
            specularAmount: 0.0
            // Slight self-illumination flattens the folds into a clay/diagram look.
            emissiveFactor: Qt.vector3d(root.baseColor.r * 0.18,
                                        root.baseColor.g * 0.18,
                                        root.baseColor.b * 0.18)
        }

        PrincipledMaterial {
            id: glowMaterial
            baseColor: root.glowColor
            roughness: 1.0
            metalness: 0.0
            specularAmount: 0.0
            emissiveFactor: Qt.vector3d(root.glowColor.r * 0.55,
                                        root.glowColor.g * 0.55,
                                        root.glowColor.b * 0.55)
            SequentialAnimation on emissiveFactor {
                running: root.ready
                loops: Animation.Infinite
                Vector3dAnimation {
                    to: Qt.vector3d(root.glowColor.r * 0.9,
                                    root.glowColor.g * 0.9,
                                    root.glowColor.b * 0.9)
                    duration: 900
                    easing.type: Easing.InOutSine
                }
                Vector3dAnimation {
                    to: Qt.vector3d(root.glowColor.r * 0.35,
                                    root.glowColor.g * 0.35,
                                    root.glowColor.b * 0.35)
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
        id: orbit
        anchors.fill: parent
        origin: originNode
        camera: camera
        // Match drag direction to how the model turns.
        xSpeed: 0.18
        ySpeed: 0.18
        xInvert: false
        yInvert: true
        panEnabled: false
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
