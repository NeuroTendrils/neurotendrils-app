#include "theme/AppFonts.hpp"

#include <QApplication>
#include <QFontDatabase>

namespace {

// Each static Geist weight ships as its own font file. Their name tables
// don't always resolve to an identical family string across platforms, so
// we resolve and cache the family reported for each individual face rather
// than assuming Qt's weight matcher will find the right one under a single
// shared family name. That avoids Qt falling back to a synthesized
// (faux) bold/semibold when it can't find an exact face, which renders
// noticeably heavier and less crisp than the real weight.
QString gRegularFamily = QStringLiteral("Geist");
QString gMediumFamily = QStringLiteral("Geist");
QString gSemiboldFamily = QStringLiteral("Geist");
QString gBoldFamily = QStringLiteral("Geist");

QString loadFont(const char* resourcePath) {
    const int fontId = QFontDatabase::addApplicationFont(QString::fromLatin1(resourcePath));
    if (fontId < 0) {
        return {};
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    return families.isEmpty() ? QString() : families.constLast();
}

QString fallbackFamily() {
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
}

QFont makeFont(const QString& family, int pointSize, QFont::Weight fallbackWeight) {
    QFont font(family.isEmpty() ? fallbackFamily() : family);
    font.setPointSize(pointSize);
    font.setWeight(fallbackWeight);
    font.setHintingPreference(QFont::PreferFullHinting);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

} // namespace

namespace AppFonts {

void initialize(QApplication& app) {
    const QString regularFamily = loadFont(":/fonts/Geist-Regular.ttf");
    const QString semiboldFamily = loadFont(":/fonts/Geist-SemiBold.ttf");
    const QString boldFamily = loadFont(":/fonts/Geist-Bold.ttf");
    const QString systemFamily = fallbackFamily();

    gRegularFamily = regularFamily.isEmpty() ? systemFamily : regularFamily;
    gMediumFamily = gRegularFamily;
    gSemiboldFamily = semiboldFamily.isEmpty() ? gRegularFamily : semiboldFamily;
    gBoldFamily = boldFamily.isEmpty() ? gRegularFamily : boldFamily;

    app.setFont(regular(14));
}

QString family() {
    return gRegularFamily;
}

QFont regular(int pointSize) {
    return makeFont(gRegularFamily, pointSize, QFont::Normal);
}

QFont medium(int pointSize) {
    return makeFont(gMediumFamily, pointSize, QFont::Medium);
}

QFont semibold(int pointSize) {
    return makeFont(gSemiboldFamily, pointSize, QFont::DemiBold);
}

QFont bold(int pointSize) {
    return makeFont(gBoldFamily, pointSize, QFont::Bold);
}

} // namespace AppFonts
