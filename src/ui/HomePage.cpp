#include "ui/HomePage.hpp"

#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QtGlobal>

namespace {

constexpr int kWordmarkMinPointSize = 56;
constexpr int kWordmarkMaxPointSize = 120;
constexpr int kNavButtonWidth = 320;
constexpr int kNavButtonHeight = 56;
constexpr int kNavButtonSpacing = 16;

constexpr int kWordmarkToButtonsSpacing = 48;

QPushButton* createNavButton(const QString& label, const QString& description, QWidget* parent) {
    auto* button = new QPushButton(label, parent);
    button->setObjectName(QStringLiteral("nav-button"));
    button->setFixedSize(kNavButtonWidth, kNavButtonHeight);
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(AppFonts::semibold(16));
    button->setAccessibleName(label);
    button->setAccessibleDescription(description);
    button->setToolTip(description);
    return button;
}

} // namespace

HomePage::HomePage(ThemeManager& themeManager, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager) {
    setObjectName(QStringLiteral("home-page"));
    setAttribute(Qt::WA_StyledBackground, true);
    buildUi();
    applyTheme();
    updateWordmarkFont();

    connect(&themeManager_, &ThemeManager::themeChanged, this, &HomePage::applyTheme);
}

void HomePage::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(48, 48, 48, 64);
    rootLayout->setSpacing(0);

    rootLayout->addStretch(4);

    auto* wordmarkRow = new QHBoxLayout();
    wordmarkRow->setSpacing(0);
    wordmarkRow->addStretch();

    neuroLabel_ = new QLabel(QStringLiteral("Neuro"), this);
    neuroLabel_->setObjectName(QStringLiteral("wordmark-neuro"));
    neuroLabel_->setAccessibleName(QStringLiteral("NeuroTendrils"));

    tendrilsLabel_ = new QLabel(QStringLiteral("Tendrils"), this);
    tendrilsLabel_->setObjectName(QStringLiteral("wordmark-tendrils"));
    tendrilsLabel_->setAccessibleDescription(QString());

    wordmarkRow->addWidget(neuroLabel_);
    wordmarkRow->addWidget(tendrilsLabel_);
    wordmarkRow->addStretch();

    rootLayout->addLayout(wordmarkRow);
    rootLayout->addSpacing(kWordmarkToButtonsSpacing);
    rootLayout->addStretch(3);

    auto* buttonColumn = new QVBoxLayout();
    buttonColumn->setSpacing(kNavButtonSpacing);
    buttonColumn->setAlignment(Qt::AlignHCenter);

    roboticArmButton_ = createNavButton(
        QStringLiteral("Robotic Arm"),
        QStringLiteral("Robotic arm control. Coming soon."),
        this);
    eegToTextButton_ = createNavButton(
        QStringLiteral("EEG-to-Text"),
        QStringLiteral("EEG-to-text interface. Coming soon."),
        this);
    educationButton_ = createNavButton(
        QStringLiteral("Education"),
        QStringLiteral("Explore the brain-computer interface and control the arm."),
        this);

    buttonColumn->addWidget(roboticArmButton_, 0, Qt::AlignHCenter);
    buttonColumn->addWidget(eegToTextButton_, 0, Qt::AlignHCenter);
    buttonColumn->addWidget(educationButton_, 0, Qt::AlignHCenter);

    connect(educationButton_, &QPushButton::clicked, this, &HomePage::educationRequested);

    rootLayout->addLayout(buttonColumn);
    rootLayout->addStretch(4);
}

void HomePage::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QString background = colors.background.name(QColor::HexRgb);

    setStyleSheet(QStringLiteral("QWidget#home-page { background-color: %1; }").arg(background));

    if (neuroLabel_ != nullptr) {
        neuroLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
            .arg(colors.accent.name(QColor::HexRgb)));
    }

    if (tendrilsLabel_ != nullptr) {
        tendrilsLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
            .arg(colors.foreground.name(QColor::HexRgb)));
    }

    const QString buttonStyle = navButtonStylesheet();
    if (roboticArmButton_ != nullptr) {
        roboticArmButton_->setStyleSheet(buttonStyle);
    }
    if (eegToTextButton_ != nullptr) {
        eegToTextButton_->setStyleSheet(buttonStyle);
    }
    if (educationButton_ != nullptr) {
        educationButton_->setStyleSheet(buttonStyle);
    }
}

QString HomePage::navButtonStylesheet() const {
    const ColorPalette colors = themeManager_.palette();
    const QString elevated = colors.backgroundElevated.name(QColor::HexRgb);
    const QString foreground = colors.foreground.name(QColor::HexRgb);
    const QString border = colors.border.name(QColor::HexArgb);
    const QString accent = colors.accent.name(QColor::HexRgb);
    const QString accentSubtle = colors.accentSubtle.name(QColor::HexArgb);
    const QString accentStrong = colors.accentStrong.name(QColor::HexRgb);

    return QStringLiteral(
               "QPushButton#nav-button {"
               "  background-color: %1;"
               "  color: %2;"
               "  border: 1px solid %3;"
               "  border-radius: 10px;"
               "  padding: 0 20px;"
               "}"
               "QPushButton#nav-button:hover {"
               "  background-color: %4;"
               "  border-color: %5;"
               "  color: %5;"
               "}"
               "QPushButton#nav-button:pressed {"
               "  background-color: %4;"
               "  border-color: %6;"
               "  color: %6;"
               "}"
               "QPushButton#nav-button:focus {"
               "  border-color: %5;"
               "  border-width: 2px;"
               "}")
        .arg(elevated, foreground, border, accentSubtle, accent, accentStrong);
}

void HomePage::updateWordmarkFont() {
    const int scaleBasis = qMin(width(), height());
    const int scaledSize = qBound(kWordmarkMinPointSize, scaleBasis / 8, kWordmarkMaxPointSize);

    QFont wordmarkFont = AppFonts::semibold(scaledSize);
    wordmarkFont.setLetterSpacing(QFont::AbsoluteSpacing, scaledSize * -0.035);

    if (neuroLabel_ != nullptr) {
        neuroLabel_->setFont(wordmarkFont);
    }

    if (tendrilsLabel_ != nullptr) {
        tendrilsLabel_->setFont(wordmarkFont);
    }
}

void HomePage::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateWordmarkFont();
}
