import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.AssetUtils

// Interactive cortical atlas. The brain model is loaded at runtime and every
// mesh gets a shared neutral material; the active region's meshes swap to a
// glowing accent material. Floating label cards project from region anchors
// (same approach as the web demo).
Item {
    id: root

    property url modelSource: ""
    property color baseColor: "#b5aea0"
    property color glowColor: "#6f96ff"
    property color backgroundColor: "#ffffff"
    property var highlightIndices: []
    property vector3d modelCenter: Qt.vector3d(0.0, 1.619, -0.009)
    property real modelSize: 0.181
    property bool autoRotate: true
    property bool ready: false
    property string loadError: ""

    // [{ id, title, description, color, motions, indices: [int] }]
    property var regionLabels: []
    property string activeRegionId: ""

    property color labelCardBackground: "#fffffff5"
    property color labelCardBorder: "#d8dde8"
    property color labelCardTitle: "#1a1f36"
    property color labelCardBody: "#5c6478"

    property var _models: []
    property var _labelLayouts: []
    // "full" | "compact" | "hidden" — driven by viewport size in _layoutLabels.
    property string labelMode: "full"

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
        visible: !root.ready
        z: 10

        Column {
            anchors.centerIn: parent
            spacing: 8
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.loadError.length > 0 ? "Brain failed to load" : "Loading atlas…"
                color: "#97a0b5"
                font.pixelSize: 14
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(root.width - 48, 420)
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                visible: root.loadError.length > 0
                text: root.loadError
                color: "#97a0b5"
                font.pixelSize: 12
            }
        }
    }

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

            NumberAnimation on eulerRotation.y {
                running: root.autoRotate && root.ready && !orbit.inputsNeedProcessing
                from: originNode.eulerRotation.y
                to: originNode.eulerRotation.y + 360
                duration: 44000
                loops: Animation.Infinite
            }
        }

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
                    root.loadError = ""
                    root._collectModels()
                    root._normalizeToView()
                    root._applyHighlight()
                    root.ready = true
                    root._layoutLabels()
                } else if (status === RuntimeLoader.Error) {
                    root.loadError = errorString
                    root.ready = false
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
        xSpeed: 0.18
        ySpeed: 0.18
        xInvert: false
        yInvert: true
        panEnabled: false
    }

    // Screen-space label overlay (cards + pins + leader lines).
    // Hidden entirely on small viewports — the status bar under the brain
    // carries the active-region copy instead of orphan pins.
    Item {
        id: labelsLayer
        anchors.fill: parent
        visible: root.ready && root.labelMode !== "hidden"
        z: 5
        enabled: false

        Canvas {
            id: lineCanvas
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (root.labelMode === "hidden")
                    return
                for (var i = 0; i < root._labelLayouts.length; ++i) {
                    var entry = root._labelLayouts[i]
                    if (!entry.visible || !entry.showCard)
                        continue
                    ctx.beginPath()
                    ctx.strokeStyle = entry.color
                    ctx.globalAlpha = entry.dimmed ? 0.2 : (entry.active ? 0.85 : 0.5)
                    ctx.lineWidth = entry.active ? 1.75 : 1.5
                    ctx.moveTo(entry.attachX, entry.attachY)
                    ctx.lineTo(entry.pinX, entry.pinY)
                    ctx.stroke()
                    ctx.globalAlpha = 1.0
                }
            }
        }

        Repeater {
            model: root.regionLabels

            Item {
                id: labelRoot
                property var layout: index < root._labelLayouts.length ? root._labelLayouts[index] : null
                property real cardW: layout ? layout.cardWidth : 200
                property real cardH: Math.max(cardColumn.implicitHeight + 20, 56)

                width: cardW
                height: cardH
                x: layout ? layout.cardX : -1000
                y: layout ? layout.cardY : -1000
                visible: layout ? (layout.visible && layout.showCard) : false
                opacity: !layout ? 0 : (layout.dimmed ? 0.4 : (layout.active ? 1.0 : 0.96))
                z: layout && layout.active ? 3 : 1

                Rectangle {
                    id: card
                    anchors.fill: parent
                    radius: 8
                    color: root.labelCardBackground
                    border.width: 1
                    border.color: root.labelCardBorder

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 3
                        color: layout ? layout.color : "#6f96ff"
                    }

                    Column {
                        id: cardColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 8
                        anchors.leftMargin: 12
                        spacing: 3

                        Text {
                            width: parent.width
                            text: modelData.motions || ""
                            color: layout ? layout.color : "#6f96ff"
                            font.pixelSize: 10
                            font.bold: true
                            font.letterSpacing: 0.4
                            wrapMode: Text.WordWrap
                            visible: text.length > 0
                        }
                        Text {
                            width: parent.width
                            text: modelData.title || ""
                            color: root.labelCardTitle
                            font.pixelSize: 12
                            font.bold: true
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            width: parent.width
                            text: modelData.description || ""
                            color: root.labelCardBody
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                            visible: root.labelMode === "full"
                        }
                    }
                }

                onCardHChanged: {
                    if (layout)
                        layout.cardHeight = cardH
                }
            }
        }

        Repeater {
            model: root.regionLabels

            Rectangle {
                property var layout: index < root._labelLayouts.length ? root._labelLayouts[index] : null
                width: 11
                height: 11
                radius: 5.5
                color: layout ? layout.color : "#6f96ff"
                x: layout ? (layout.pinX - width / 2) : -1000
                y: layout ? (layout.pinY - height / 2) : -1000
                visible: layout ? (layout.visible && layout.showCard) : false
                opacity: !layout ? 0 : (layout.dimmed ? 0.35 : 0.95)
                z: 6

                Rectangle {
                    anchors.centerIn: parent
                    width: 19
                    height: 19
                    radius: 9.5
                    color: parent.color
                    opacity: 0.28
                    z: -1
                }
            }
        }
    }

    Timer {
        interval: 33
        running: root.ready
        repeat: true
        onTriggered: root._layoutLabels()
    }

    onHighlightIndicesChanged: _applyHighlight()
    onModelSizeChanged: if (root.ready) _normalizeToView()
    onRegionLabelsChanged: if (root.ready) _layoutLabels()
    onActiveRegionIdChanged: if (root.ready) _layoutLabels()
    onWidthChanged: if (root.ready) _layoutLabels()
    onHeightChanged: if (root.ready) _layoutLabels()

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

    function _meshName(model) {
        if (!model)
            return ""
        if (model.objectName && model.objectName.length)
            return model.objectName
        return ""
    }

    function _preferHemisphere(indices) {
        var left = []
        var right = []
        var all = []
        for (var i = 0; i < indices.length; ++i) {
            var idx = indices[i]
            if (idx < 0 || idx >= _models.length)
                continue
            all.push(idx)
            var name = _meshName(_models[idx])
            if (name.indexOf(".l") !== -1)
                left.push(idx)
            else if (name.indexOf(".r") !== -1)
                right.push(idx)
        }
        if (left.length > 0)
            return left
        if (right.length > 0)
            return right
        return all
    }

    function _regionSceneCenter(region) {
        // Prefer the C++-computed model-space anchor (survives Draco / empty
        // RuntimeLoader bounds). Transform with the same scale/position as the loader.
        if (region.anchor !== undefined && region.anchor !== null) {
            var a = region.anchor
            var s = loader.scale
            var p = loader.position
            return Qt.vector3d(a.x * s.x + p.x, a.y * s.y + p.y, a.z * s.z + p.z)
        }

        var indices = region.indices || []
        var aim = _preferHemisphere(indices)
        var any = false
        var minX = 1e9, minY = 1e9, minZ = 1e9
        var maxX = -1e9, maxY = -1e9, maxZ = -1e9

        for (var i = 0; i < aim.length; ++i) {
            var model = _models[aim[i]]
            if (!model || model.bounds === undefined)
                continue
            var b = model.bounds
            var corners = [
                Qt.vector3d(b.minimum.x, b.minimum.y, b.minimum.z),
                Qt.vector3d(b.maximum.x, b.maximum.y, b.maximum.z)
            ]
            for (var c = 0; c < corners.length; ++c) {
                var scenePos = model.mapPositionToScene(corners[c])
                any = true
                minX = Math.min(minX, scenePos.x)
                minY = Math.min(minY, scenePos.y)
                minZ = Math.min(minZ, scenePos.z)
                maxX = Math.max(maxX, scenePos.x)
                maxY = Math.max(maxY, scenePos.y)
                maxZ = Math.max(maxZ, scenePos.z)
            }
        }

        if (!any)
            return Qt.vector3d(0, 0, 0)
        return Qt.vector3d((minX + maxX) * 0.5, (minY + maxY) * 0.5, (minZ + maxZ) * 0.5)
    }

    function _separateStacks(layouts, height) {
        var sides = { left: [], right: [], top: [] }
        for (var i = 0; i < layouts.length; ++i)
            sides[layouts[i].side].push(layouts[i])

        var gap = 10
        var sideNames = ["left", "right", "top"]
        for (var s = 0; s < sideNames.length; ++s) {
            var group = sides[sideNames[s]]
            group.sort(function (a, b) { return a.cardY - b.cardY })

            for (var j = 1; j < group.length; ++j) {
                var minY = group[j - 1].cardY + group[j - 1].cardHeight + gap
                if (group[j].cardY < minY)
                    group[j].cardY = minY
            }
            for (var k = group.length - 2; k >= 0; --k) {
                var maxY = group[k + 1].cardY - group[k].cardHeight - gap
                if (group[k].cardY > maxY)
                    group[k].cardY = Math.max(8, maxY)
            }
            for (var m = 0; m < group.length; ++m) {
                var e = group[m]
                e.cardY = Math.min(Math.max(e.cardY, 8), height - e.cardHeight - 8)
                if (e.side === "left") {
                    e.attachX = e.cardX + e.cardWidth
                    e.attachY = e.cardY + e.cardHeight * 0.5
                } else if (e.side === "right") {
                    e.attachX = e.cardX
                    e.attachY = e.cardY + e.cardHeight * 0.5
                } else {
                    e.attachX = e.cardX + e.cardWidth * 0.5
                    e.attachY = e.cardY + e.cardHeight
                }
            }
        }
    }

    function _layoutLabels() {
        if (!ready || regionLabels === undefined || regionLabels.length === 0) {
            _labelLayouts = []
            lineCanvas.requestPaint()
            return
        }

        var width = root.width
        var height = root.height

        if (width < 480 || height < 320)
            labelMode = "hidden"
        else if (width < 640 || height < 420)
            labelMode = "compact"
        else
            labelMode = "full"

        var showCards = labelMode !== "hidden"
        var layouts = []
        var cardWidth = Math.min(220, Math.max(150, width * 0.26))
        var cardHeight = labelMode === "full" ? 88 : 64

        for (var i = 0; i < regionLabels.length; ++i) {
            var region = regionLabels[i]
            var center = _regionSceneCenter(region)
            var projected = view.mapFrom3DScene(center)

            var visible = projected.z > 0
                    && projected.x > -80 && projected.x < width + 80
                    && projected.y > -80 && projected.y < height + 80

            var pinX = projected.x
            var pinY = projected.y
            var side = pinX < width * 0.5 ? "left" : "right"
            if (pinY < height * 0.18 && pinX > width * 0.28 && pinX < width * 0.72)
                side = "top"

            if (i < _labelLayouts.length && _labelLayouts[i].cardHeight)
                cardHeight = Math.max(labelMode === "full" ? 72 : 52, _labelLayouts[i].cardHeight)

            var cardX = 10
            var cardY = pinY - cardHeight * 0.5
            if (side === "left")
                cardX = 8
            else if (side === "right")
                cardX = width - cardWidth - 8
            else {
                cardX = Math.min(Math.max(pinX - cardWidth * 0.5, 8), width - cardWidth - 8)
                cardY = 8
            }

            var active = activeRegionId.length > 0 && region.id === activeRegionId
            var dimmed = activeRegionId.length > 0 && !active

            layouts.push({
                id: region.id,
                title: region.title || "",
                description: region.description || "",
                motions: region.motions || "",
                color: region.color || "#6f96ff",
                visible: visible,
                showCard: showCards,
                active: active,
                dimmed: dimmed,
                side: side,
                pinX: pinX,
                pinY: pinY,
                cardX: cardX,
                cardY: cardY,
                cardWidth: cardWidth,
                cardHeight: cardHeight,
                attachX: 0,
                attachY: 0
            })
        }

        if (showCards)
            _separateStacks(layouts, height)
        _labelLayouts = layouts
        lineCanvas.requestPaint()
    }
}
