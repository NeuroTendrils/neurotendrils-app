#pragma once

#include <QFont>

class QApplication;

namespace AppFonts {

void initialize(QApplication& app);

QString family();

QFont regular(int pointSize);
QFont medium(int pointSize);
QFont semibold(int pointSize);
QFont bold(int pointSize);

} // namespace AppFonts
