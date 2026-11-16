#include "duifx/Palette.h"

#include <array>
#include <utility>

namespace duifx {

namespace palette {

QMap<QString, QString> colorDict()
{
    const std::array<std::pair<const char *, const ColorScale *>, 8> scales{{
        {"gray", &gray},
        {"blue", &blue},
        {"green", &green},
        {"red", &red},
        {"yellow", &yellow},
        {"purple", &purple},
        {"orange", &orange},
        {"teal", &teal},
    }};
    QMap<QString, QString> colors;
    for (const auto &[name, scale] : scales) {
        const std::array<std::pair<int, const char *>, 10> shades{{
            {50, scale->s50},
            {100, scale->s100},
            {200, scale->s200},
            {300, scale->s300},
            {400, scale->s400},
            {500, scale->s500},
            {600, scale->s600},
            {700, scale->s700},
            {800, scale->s800},
            {900, scale->s900},
        }};
        for (const auto &[shade, value] : shades)
            colors.insert(QStringLiteral("%1_%2").arg(QLatin1StringView(name)).arg(shade), QString::fromLatin1(value));
    }
    colors.insert(QStringLiteral("white"), QString::fromLatin1(white));
    colors.insert(QStringLiteral("black"), QString::fromLatin1(black));
    colors.insert(QStringLiteral("primary"), QString::fromLatin1(primary));
    colors.insert(QStringLiteral("secondary"), QString::fromLatin1(secondary));
    colors.insert(QStringLiteral("success"), QString::fromLatin1(success));
    colors.insert(QStringLiteral("warning"), QString::fromLatin1(warning));
    colors.insert(QStringLiteral("danger"), QString::fromLatin1(danger));
    colors.insert(QStringLiteral("info"), QString::fromLatin1(info));
    return colors;
}

} // namespace palette

namespace base {

QMap<QString, QString> colorDict()
{
    QMap<QString, QString> colors = palette::colorDict();
    const std::array<std::pair<const char *, const char *>, 14> extras{{
        {"ink", ink},
        {"chrome", chrome},
        {"signal_green", signalGreen},
        {"signal_orange", signalOrange},
        {"signal_red", signalRed},
        {"plot_red", plotRed},
        {"plot_green", plotGreen},
        {"plot_blue", plotBlue},
        {"plot_amber", plotAmber},
        {"plot_pink", plotPink},
        {"plot_mint", plotMint},
        {"plot_light", plotLight},
        {"plot_axis", plotAxis},
        {"plot_black", plotBlack},
    }};
    for (const auto &[name, value] : extras)
        colors.insert(QString::fromLatin1(name), QString::fromLatin1(value));
    return colors;
}

} // namespace base

} // namespace duifx
