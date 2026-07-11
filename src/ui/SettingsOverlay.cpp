#include "ui/SettingsOverlay.hpp"

#include "AppVersion.hpp"
#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"

#include <QButtonGroup>
#include <QDesktopServices>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {

constexpr int kPanelWidth = 620;
constexpr int kPanelHeight = 400;
constexpr int kSidebarWidth = 170;
constexpr char kWebsiteUrl[] = "https://neurotendrils.com";

QWidget* createPreviewBar(const QString& color, int height) {
    auto* bar = new QLabel();
    bar->setFixedHeight(height);
    bar->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 2px;").arg(color));
    bar->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    return bar;
}

QWidget* createThemePreview(Theme previewTheme) {
    auto* preview = new QWidget();
    preview->setObjectName(QStringLiteral("theme-card-preview"));
    preview->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    if (previewTheme == Theme::Auto) {
        preview->setFixedHeight(52);
        preview->setStyleSheet(QStringLiteral(
            "QWidget#theme-card-preview {"
            "  border: 1px solid rgba(26, 31, 54, 0.18);"
            "  border-radius: 7px;"
            "}"));

        auto* splitLayout = new QHBoxLayout(preview);
        splitLayout->setContentsMargins(0, 0, 0, 0);
        splitLayout->setSpacing(0);

        auto* lightHalf = new QWidget();
        lightHalf->setStyleSheet(QStringLiteral(
            "background-color: #ffffff;"
            "border-top-left-radius: 6px;"
            "border-bottom-left-radius: 6px;"));
        lightHalf->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto* darkHalf = new QWidget();
        darkHalf->setStyleSheet(QStringLiteral(
            "background-color: #12172a;"
            "border-top-right-radius: 6px;"
            "border-bottom-right-radius: 6px;"));
        darkHalf->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        splitLayout->addWidget(lightHalf);
        splitLayout->addWidget(darkHalf);
        return preview;
    }

    const bool isLight = previewTheme == Theme::Light;
    const QString background = isLight ? QStringLiteral("#ffffff") : QStringLiteral("#12172a");
    const QString border = isLight ? QStringLiteral("rgba(26, 31, 54, 0.15)")
                                   : QStringLiteral("rgba(231, 234, 242, 0.12)");
    const QString accent = isLight ? QStringLiteral("#2b59c3") : QStringLiteral("#6f96ff");
    const QString mutedBar = isLight ? QStringLiteral("#e8ecf4") : QStringLiteral("#1e2640");

    preview->setFixedHeight(52);
    preview->setStyleSheet(QStringLiteral(
                               "QWidget#theme-card-preview {"
                               "  background-color: %1;"
                               "  border: 1px solid %2;"
                               "  border-radius: 7px;"
                               "}")
                               .arg(background, border));

    auto* previewLayout = new QVBoxLayout(preview);
    previewLayout->setContentsMargins(10, 10, 10, 10);
    previewLayout->setSpacing(5);
    previewLayout->addWidget(createPreviewBar(accent, 5));
    previewLayout->addWidget(createPreviewBar(mutedBar, 4));
    previewLayout->addStretch();

    return preview;
}

QPushButton* createThemeCard(
    const QString& label,
    Theme previewTheme,
    const QString& accessibleDescription,
    QWidget* parent) {
    auto* card = new QPushButton(parent);
    card->setObjectName(QStringLiteral("theme-card"));
    card->setCheckable(true);
    card->setCursor(Qt::PointingHandCursor);
    card->setFixedSize(118, 98);
    card->setAccessibleName(label);
    card->setAccessibleDescription(accessibleDescription);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(8, 8, 8, 8);
    cardLayout->setSpacing(6);

    auto* labelWidget = new QLabel(label, card);
    labelWidget->setObjectName(QStringLiteral("theme-card-label"));
    labelWidget->setFont(AppFonts::medium(12));
    labelWidget->setAlignment(Qt::AlignCenter);
    labelWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    cardLayout->addWidget(createThemePreview(previewTheme));
    cardLayout->addWidget(labelWidget);

    return card;
}

} // namespace

SettingsOverlay::SettingsOverlay(ThemeManager& themeManager, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager) {
    setObjectName(QStringLiteral("settings-overlay"));
    setAccessibleName(QStringLiteral("Settings"));
    setAccessibleDescription(QStringLiteral("Application settings. Press Escape to close."));
    setFocusPolicy(Qt::StrongFocus);
    hide();

    buildUi();
    syncThemeSelection(themeManager_.currentTheme());
    showAppearancePage();
    applyTheme();

    connect(&themeManager_, &ThemeManager::themeChanged, this, &SettingsOverlay::syncThemeSelection);
    connect(&themeManager_, &ThemeManager::themeChanged, this, &SettingsOverlay::applyTheme);

    if (parent != nullptr) {
        parent->installEventFilter(this);
    }
}

void SettingsOverlay::buildUi() {
    backdrop_ = new QWidget(this);
    backdrop_->setObjectName(QStringLiteral("settings-backdrop"));

    panel_ = new QWidget(this);
    panel_->setObjectName(QStringLiteral("settings-panel"));
    panel_->setFixedSize(kPanelWidth, kPanelHeight);
    panel_->setAttribute(Qt::WA_StyledBackground, true);
    panel_->setFocusPolicy(Qt::StrongFocus);

    auto* shadow = new QGraphicsDropShadowEffect(panel_);
    shadow->setBlurRadius(48);
    shadow->setOffset(0, 14);
    shadow->setColor(QColor(0, 0, 0, 90));
    panel_->setGraphicsEffect(shadow);

    auto* panelLayout = new QHBoxLayout(panel_);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);

    sidebar_ = new QWidget(panel_);
    sidebar_->setObjectName(QStringLiteral("settings-sidebar"));
    sidebar_->setFixedWidth(kSidebarWidth);

    auto* sidebarLayout = new QVBoxLayout(sidebar_);
    sidebarLayout->setContentsMargins(20, 24, 12, 20);
    sidebarLayout->setSpacing(2);

    auto* settingsTitle = new QLabel(QStringLiteral("Settings"), sidebar_);
    settingsTitle->setObjectName(QStringLiteral("settings-title"));
    settingsTitle->setFont(AppFonts::semibold(13));

    appearanceNav_ = new QLabel(QStringLiteral("Appearance"), sidebar_);
    appearanceNav_->setCursor(Qt::PointingHandCursor);
    appearanceNav_->setAttribute(Qt::WA_Hover, true);
    appearanceNav_->setAccessibleName(QStringLiteral("Appearance"));
    appearanceNav_->setAccessibleDescription(QStringLiteral("Show theme and display settings"));
    appearanceNav_->installEventFilter(this);

    infoNav_ = new QLabel(QStringLiteral("Info"), sidebar_);
    infoNav_->setCursor(Qt::PointingHandCursor);
    infoNav_->setAttribute(Qt::WA_Hover, true);
    infoNav_->setAccessibleName(QStringLiteral("Info"));
    infoNav_->setAccessibleDescription(QStringLiteral("Show application version and website"));
    infoNav_->installEventFilter(this);

    sidebarLayout->addWidget(settingsTitle);
    sidebarLayout->addSpacing(16);
    sidebarLayout->addWidget(appearanceNav_);
    sidebarLayout->addWidget(infoNav_);
    sidebarLayout->addStretch();

    contentArea_ = new QWidget(panel_);
    contentArea_->setObjectName(QStringLiteral("settings-content"));

    auto* contentLayout = new QVBoxLayout(contentArea_);
    contentLayout->setContentsMargins(32, 30, 32, 30);
    contentLayout->setSpacing(0);

    contentStack_ = new QStackedWidget(contentArea_);
    contentStack_->setObjectName(QStringLiteral("settings-stack"));

    auto* appearancePage = new QWidget(contentStack_);
    auto* appearanceLayout = new QVBoxLayout(appearancePage);
    appearanceLayout->setContentsMargins(0, 0, 0, 0);
    appearanceLayout->setSpacing(22);

    auto* appearanceHeading = new QLabel(QStringLiteral("Appearance"), appearancePage);
    appearanceHeading->setObjectName(QStringLiteral("settings-content-title"));
    appearanceHeading->setFont(AppFonts::semibold(21));

    auto* themeSection = new QWidget(appearancePage);
    auto* themeSectionLayout = new QVBoxLayout(themeSection);
    themeSectionLayout->setContentsMargins(0, 0, 0, 0);
    themeSectionLayout->setSpacing(12);

    auto* themeLabel = new QLabel(QStringLiteral("Theme"), themeSection);
    themeLabel->setObjectName(QStringLiteral("theme-label"));
    themeLabel->setFont(AppFonts::medium(14));

    auto* themeCards = new QWidget(themeSection);
    auto* themeCardsLayout = new QHBoxLayout(themeCards);
    themeCardsLayout->setContentsMargins(0, 0, 0, 0);
    themeCardsLayout->setSpacing(12);

    themeGroup_ = new QButtonGroup(themeCards);
    themeGroup_->setExclusive(true);

    lightThemeCard_ = createThemeCard(
        QStringLiteral("Light"),
        Theme::Light,
        QStringLiteral("Always use the light theme"),
        themeCards);
    darkThemeCard_ = createThemeCard(
        QStringLiteral("Dark"),
        Theme::Dark,
        QStringLiteral("Always use the dark theme"),
        themeCards);
    autoThemeCard_ = createThemeCard(
        QStringLiteral("Auto"),
        Theme::Auto,
        QStringLiteral("Match the system light or dark appearance"),
        themeCards);

    themeGroup_->addButton(lightThemeCard_);
    themeGroup_->addButton(darkThemeCard_);
    themeGroup_->addButton(autoThemeCard_);

    connect(lightThemeCard_, &QPushButton::clicked, this, [this]() {
        themeManager_.setTheme(Theme::Light);
    });

    connect(darkThemeCard_, &QPushButton::clicked, this, [this]() {
        themeManager_.setTheme(Theme::Dark);
    });

    connect(autoThemeCard_, &QPushButton::clicked, this, [this]() {
        themeManager_.setTheme(Theme::Auto);
    });

    themeCardsLayout->addWidget(lightThemeCard_);
    themeCardsLayout->addWidget(darkThemeCard_);
    themeCardsLayout->addWidget(autoThemeCard_);
    themeCardsLayout->addStretch();

    themeSectionLayout->addWidget(themeLabel);
    themeSectionLayout->addWidget(themeCards);

    appearanceLayout->addWidget(appearanceHeading);
    appearanceLayout->addWidget(themeSection);
    appearanceLayout->addStretch();

    auto* infoPage = new QWidget(contentStack_);
    auto* infoLayout = new QVBoxLayout(infoPage);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(22);

    auto* infoHeading = new QLabel(QStringLiteral("Info"), infoPage);
    infoHeading->setObjectName(QStringLiteral("settings-content-title"));
    infoHeading->setFont(AppFonts::semibold(21));

    auto* appIdentity = new QWidget(infoPage);
    auto* appIdentityLayout = new QVBoxLayout(appIdentity);
    appIdentityLayout->setContentsMargins(0, 0, 0, 0);
    appIdentityLayout->setSpacing(4);

    auto* appNameLabel = new QLabel(QStringLiteral("NeuroTendrils"), appIdentity);
    appNameLabel->setObjectName(QStringLiteral("info-app-name"));
    appNameLabel->setFont(AppFonts::semibold(15));

    auto* versionLabel = new QLabel(
        QStringLiteral("Version %1").arg(QString::fromLatin1(NT_APP_VERSION)),
        appIdentity);
    versionLabel->setObjectName(QStringLiteral("info-secondary"));
    versionLabel->setFont(AppFonts::regular(13));

    appIdentityLayout->addWidget(appNameLabel);
    appIdentityLayout->addWidget(versionLabel);

    auto* websiteLink = new QLabel(infoPage);
    websiteLink->setObjectName(QStringLiteral("website-link"));
    websiteLink->setFont(AppFonts::regular(13));
    websiteLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
    websiteLink->setOpenExternalLinks(true);
    websiteLink->setCursor(Qt::PointingHandCursor);
    websiteLink->setText(QStringLiteral("<a href=\"%1\">%1</a>").arg(QString::fromLatin1(kWebsiteUrl)));
    websiteLink->setAccessibleName(QStringLiteral("NeuroTendrils website"));
    websiteLink->setAccessibleDescription(QStringLiteral("Opens neurotendrils.com in your browser"));

    infoLayout->addWidget(infoHeading);
    infoLayout->addWidget(appIdentity);
    infoLayout->addWidget(websiteLink);
    infoLayout->addStretch();

    contentStack_->addWidget(appearancePage);
    contentStack_->addWidget(infoPage);

    contentLayout->addWidget(contentStack_);

    panelLayout->addWidget(sidebar_);
    panelLayout->addWidget(contentArea_, 1);

    backdrop_->installEventFilter(this);
}

void SettingsOverlay::showAppearancePage() {
    if (appearanceNav_ != nullptr) {
        appearanceNav_->setObjectName(QStringLiteral("settings-nav-active"));
        appearanceNav_->setFont(AppFonts::medium(14));
    }
    if (infoNav_ != nullptr) {
        infoNav_->setObjectName(QStringLiteral("settings-nav-inactive"));
        infoNav_->setFont(AppFonts::regular(14));
    }
    if (contentStack_ != nullptr) {
        contentStack_->setCurrentIndex(0);
    }
    applyTheme();
}

void SettingsOverlay::showInfoPage() {
    if (appearanceNav_ != nullptr) {
        appearanceNav_->setObjectName(QStringLiteral("settings-nav-inactive"));
        appearanceNav_->setFont(AppFonts::regular(14));
    }
    if (infoNav_ != nullptr) {
        infoNav_->setObjectName(QStringLiteral("settings-nav-active"));
        infoNav_->setFont(AppFonts::medium(14));
    }
    if (contentStack_ != nullptr) {
        contentStack_->setCurrentIndex(1);
    }
    applyTheme();
}

void SettingsOverlay::openPanel() {
    if (open_) {
        return;
    }

    open_ = true;
    showAppearancePage();
    raise();
    show();
    layoutPanel();
    setFocus(Qt::PopupFocusReason);
    if (panel_ != nullptr) {
        panel_->setFocus(Qt::PopupFocusReason);
    }
    emit opened();
}

void SettingsOverlay::closePanel() {
    if (!open_) {
        return;
    }

    open_ = false;
    hide();
    emit closed();
}

bool SettingsOverlay::isOpen() const {
    return open_;
}

void SettingsOverlay::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    layoutPanel();
}

void SettingsOverlay::keyPressEvent(QKeyEvent* event) {
    if (open_ && event->key() == Qt::Key_Escape) {
        closePanel();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void SettingsOverlay::layoutPanel() {
    if (backdrop_ != nullptr) {
        backdrop_->setGeometry(rect());
    }

    if (panel_ != nullptr) {
        const int x = (width() - panel_->width()) / 2;
        const int y = (height() - panel_->height()) / 2;
        panel_->setGeometry(x, y, panel_->width(), panel_->height());
    }
}

bool SettingsOverlay::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parent() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
        layoutPanel();
    }

    if (watched == backdrop_ && event->type() == QEvent::MouseButtonPress) {
        closePanel();
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        if (watched == appearanceNav_) {
            showAppearancePage();
            return true;
        }
        if (watched == infoNav_) {
            showInfoPage();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

QString SettingsOverlay::panelStylesheet() const {
    const ColorPalette colors = themeManager_.palette();

    const QString backdrop = themeManager_.effectiveTheme() == Theme::Dark
        ? QStringLiteral("rgba(2, 4, 9, 0.55)")
        : QStringLiteral("rgba(26, 31, 54, 0.16)");

    return QStringLiteral(
               "QWidget#settings-backdrop {"
               "  background-color: %1;"
               "}"
               "QWidget#settings-panel {"
               "  background-color: %2;"
               "  border: 1px solid %3;"
               "  border-radius: 14px;"
               "}"
               "QWidget#settings-sidebar {"
               "  background-color: %4;"
               "  border-right: 1px solid %3;"
               "  border-top-left-radius: 14px;"
               "  border-bottom-left-radius: 14px;"
               "}"
               "QWidget#settings-content {"
               "  background-color: %2;"
               "  border-top-right-radius: 14px;"
               "  border-bottom-right-radius: 14px;"
               "}"
               "QLabel#settings-title {"
               "  color: %5;"
               "  background: transparent;"
               "  padding: 4px 8px;"
               "}"
               "QLabel#settings-nav-active {"
               "  color: %6;"
               "  background: transparent;"
               "  padding: 7px 8px;"
               "  border-radius: 6px;"
               "}"
               "QLabel#settings-nav-inactive {"
               "  color: %5;"
               "  background: transparent;"
               "  padding: 7px 8px;"
               "  border-radius: 6px;"
               "}"
               "QLabel#settings-nav-inactive:hover {"
               "  color: %6;"
               "}"
               "QLabel#settings-content-title {"
               "  color: %6;"
               "  background: transparent;"
               "}"
               "QLabel#theme-label {"
               "  color: %6;"
               "  background: transparent;"
               "}"
               "QPushButton#theme-card {"
               "  background-color: transparent;"
               "  border: 2px solid %3;"
               "  border-radius: 10px;"
               "  padding: 0;"
               "}"
               "QPushButton#theme-card:hover {"
               "  border-color: %7;"
               "}"
               "QPushButton#theme-card:checked {"
               "  border-color: %7;"
               "  background-color: %8;"
               "}"
               "QPushButton#theme-card QLabel#theme-card-label {"
               "  color: %5;"
               "  background: transparent;"
               "}"
               "QPushButton#theme-card:checked QLabel#theme-card-label {"
               "  color: %6;"
               "}"
               "QPushButton#theme-card:focus {"
               "  border-color: %7;"
               "  outline: none;"
               "}"
               "QLabel#info-app-name {"
               "  color: %6;"
               "  background: transparent;"
               "}"
               "QLabel#info-secondary {"
               "  color: %5;"
               "  background: transparent;"
               "}"
               "QLabel#website-link {"
               "  color: %5;"
               "  background: transparent;"
               "}"
               "QLabel#website-link a {"
               "  color: %7;"
               "  text-decoration: none;"
               "}"
               "QLabel#website-link a:hover {"
               "  text-decoration: underline;"
               "}")
        .arg(
            backdrop,
            colors.backgroundElevated.name(QColor::HexRgb),
            colors.border.name(QColor::HexArgb),
            colors.backgroundSubtle.name(QColor::HexRgb),
            colors.foregroundMuted.name(QColor::HexRgb),
            colors.foreground.name(QColor::HexRgb),
            colors.accent.name(QColor::HexRgb),
            colors.accentSubtle.name(QColor::HexArgb));
}

void SettingsOverlay::applyTheme() {
    setStyleSheet(panelStylesheet());
}

void SettingsOverlay::syncThemeSelection(Theme theme) {
    QSignalBlocker lightBlocker(lightThemeCard_);
    QSignalBlocker darkBlocker(darkThemeCard_);
    QSignalBlocker autoBlocker(autoThemeCard_);

    if (lightThemeCard_ != nullptr) {
        lightThemeCard_->setChecked(theme == Theme::Light);
    }
    if (darkThemeCard_ != nullptr) {
        darkThemeCard_->setChecked(theme == Theme::Dark);
    }
    if (autoThemeCard_ != nullptr) {
        autoThemeCard_->setChecked(theme == Theme::Auto);
    }
}
