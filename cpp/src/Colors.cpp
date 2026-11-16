#include "duifx/Colors.h"

#include "duifx/Palette.h"
#include "Logging.h"

#include <QGuiApplication>
#include <QStyleHints>

#include <mutex>

namespace duifx {

namespace {

struct TokenInfo
{
    Token token;
    const char *name;
    const char *light;
    const char *dark;
};

namespace g = palette;

// Token values as (light, dark). "#aarrggbb" marks translucent ones. The dark
// column is provisional, see Colors.h.
const std::array<TokenInfo, kTokenCount> kTokens{{
    {Token::StateOk, "state_ok", base::signalGreen, base::signalGreen},
    {Token::StateBusy, "state_busy", base::signalOrange, base::signalOrange},
    {Token::StateWarning, "state_warning", base::signalOrange, base::signalOrange},
    {Token::StateError, "state_error", base::signalRed, base::signalRed},
    {Token::StateInactive, "state_inactive", g::gray.s400, g::gray.s500},
    {Token::StateInfo, "state_info", g::blue.s500, g::blue.s400},

    {Token::TextHeading, "text_heading", g::gray.s900, g::gray.s50},
    {Token::TextPrimary, "text_primary", base::ink, g::gray.s100},
    {Token::TextSecondary, "text_secondary", g::gray.s600, g::gray.s300},
    {Token::TextMuted, "text_muted", g::gray.s500, g::gray.s400},
    {Token::TextPlaceholder, "text_placeholder", g::gray.s400, g::gray.s500},
    {Token::TextDisabled, "text_disabled", g::gray.s300, g::gray.s600},
    {Token::TextWarning, "text_warning", g::yellow.s700, g::yellow.s400},
    {Token::TextOnAccent, "text_on_accent", g::white, g::white},

    {Token::Surface, "surface", g::white, g::gray.s900},
    {Token::SurfaceAlt, "surface_alt", g::gray.s50, g::gray.s800},
    {Token::SurfaceMuted, "surface_muted", base::chrome, g::gray.s800},
    {Token::Border, "border", g::gray.s300, g::gray.s600},
    {Token::BorderStrong, "border_strong", g::gray.s400, g::gray.s500},
    {Token::HighlightChanged, "highlight_changed", g::yellow.s100, g::yellow.s900},
    // A light tint of the accent: "this one is active" without the weight of
    // an accent button. Dark: translucent, so it tints any surface.
    {Token::SurfaceSelected, "surface_selected", g::blue.s100, "#403b82f6"},
    {Token::SurfaceSelectedHover, "surface_selected_hover", g::blue.s200, "#663b82f6"},

    {Token::OverlayHover, "overlay_hover", "#14ffffff", "#14ffffff"},
    {Token::OverlayPressed, "overlay_pressed", "#28ffffff", "#28ffffff"},

    {Token::Accent, "accent", g::blue.s600, g::blue.s500},
    {Token::AccentHover, "accent_hover", g::blue.s700, g::blue.s400},
    {Token::AccentPressed, "accent_pressed", g::blue.s800, g::blue.s600},
    {Token::Danger, "danger", g::red.s600, g::red.s500},
    {Token::DangerHover, "danger_hover", g::red.s700, g::red.s400},
    {Token::DangerPressed, "danger_pressed", g::red.s800, g::red.s600},
    {Token::ControlDisabled, "control_disabled", g::gray.s400, g::gray.s600},

    {Token::IndicatorOn, "indicator_on", g::gray.s900, g::gray.s100},
    {Token::IndicatorOff, "indicator_off", g::gray.s300, g::gray.s600},

    {Token::PlotSeries1, "plot_series_1", base::plotRed, base::plotRed},
    {Token::PlotSeries2, "plot_series_2", base::plotGreen, base::plotGreen},
    {Token::PlotSeries3, "plot_series_3", base::plotBlue, base::plotBlue},
    {Token::PlotSeries4, "plot_series_4", base::plotAmber, base::plotAmber},
    {Token::PlotSeries5, "plot_series_5", base::plotPink, base::plotPink},
    {Token::PlotSeries6, "plot_series_6", base::plotMint, base::plotMint},
    {Token::PlotMagnitude, "plot_magnitude", base::plotLight, base::plotLight},
    {Token::PlotBackground, "plot_background", base::plotBlack, base::plotBlack},
    {Token::PlotAxis, "plot_axis", base::plotAxis, base::plotAxis},
    // pyqtgraph draws the grid with the axis pen at alpha 0.3 (77 / 255).
    {Token::PlotGrid, "plot_grid", "#4d969696", "#4d969696"},
}};

const TokenInfo &info(Token token)
{
    const auto index = static_cast<std::size_t>(token);
    Q_ASSERT(index < kTokens.size() && kTokens[index].token == token);
    return kTokens[index];
}

std::mutex g_themeMutex;
std::optional<Theme> g_active;

Theme systemTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (QGuiApplication::instance() && QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark)
        return Theme::Dark;
#endif
    return Theme::Light;
}

/// The stored theme setting, reduced to Light or Dark.
Theme resolveTheme()
{
    Theme theme = getTheme();
    if (theme == Theme::System)
        theme = systemTheme();
    // Unimplemented themes fall back like applyTheme() does.
    if (!selectableThemes().contains(theme))
        return Theme::Light;
    return theme;
}

} // namespace

QString tokenName(Token token)
{
    return QString::fromLatin1(info(token).name);
}

std::optional<Token> tokenFromName(const QString &name)
{
    for (const TokenInfo &entry : kTokens) {
        if (name == QLatin1StringView(entry.name))
            return entry.token;
    }
    // The axis aliases have names of their own in Python only as aliases.
    if (name == QLatin1StringView("plot_axis_x"))
        return Token::PlotAxisX;
    if (name == QLatin1StringView("plot_axis_y"))
        return Token::PlotAxisY;
    if (name == QLatin1StringView("plot_axis_z"))
        return Token::PlotAxisZ;
    return std::nullopt;
}

Theme activeTheme()
{
    std::lock_guard lock(g_themeMutex);
    if (!g_active)
        g_active = resolveTheme();
    return *g_active;
}

void setActiveTheme(std::optional<Theme> theme)
{
    std::lock_guard lock(g_themeMutex);
    if (!theme)
        g_active.reset();
    else
        g_active = *theme == Theme::Dark ? Theme::Dark : Theme::Light;
}

QColor color(Token token, std::optional<int> alpha, std::optional<Theme> theme)
{
    const TokenInfo &entry = info(token);
    const Theme resolved = theme ? *theme : activeTheme();
    QColor result(QString::fromLatin1(resolved == Theme::Dark ? entry.dark : entry.light));
    if (alpha)
        result.setAlpha(*alpha);
    return result;
}

QString colorHex(Token token, std::optional<Theme> theme)
{
    return color(token, std::nullopt, theme).name();
}

QString css(Token token, std::optional<int> alpha, std::optional<Theme> theme)
{
    const QColor value = color(token, alpha, theme);
    if (value.alpha() == 255)
        return value.name();
    return QStringLiteral("rgba(%1, %2, %3, %4)").arg(value.red()).arg(value.green()).arg(value.blue()).arg(value.alpha());
}

QColor plotSeriesColor(int index, std::optional<int> alpha)
{
    const int count = static_cast<int>(kPlotSeries.size());
    const int wrapped = ((index % count) + count) % count;
    return color(kPlotSeries[static_cast<std::size_t>(wrapped)], alpha);
}

// --- StateColors ------------------------------------------------------------------

QHash<QString, Token> StateColors::defaultStateTokens()
{
    return {
        {QStringLiteral("connected"), Token::StateOk},
        {QStringLiteral("operational"), Token::StateOk},
        {QStringLiteral("online"), Token::StateOk},
        {QStringLiteral("active"), Token::StateOk},
        {QStringLiteral("ok"), Token::StateOk},
        {QStringLiteral("scanning"), Token::StateBusy},
        {QStringLiteral("connecting"), Token::StateBusy},
        {QStringLiteral("warning"), Token::StateWarning},
        {QStringLiteral("error"), Token::StateError},
        {QStringLiteral("fault"), Token::StateError},
        {QStringLiteral("heartbeat lost"), Token::StateError},
        {QStringLiteral("disconnected"), Token::StateInactive},
        {QStringLiteral("offline"), Token::StateInactive},
        {QStringLiteral("inactive"), Token::StateInactive},
        {QStringLiteral("unknown"), Token::StateInactive},
    };
}

StateColors::StateColors(const QHash<QString, Token> &mapping, Token defaultToken)
    : m_tokens(defaultStateTokens())
    , m_default(defaultToken)
{
    for (auto it = mapping.cbegin(); it != mapping.cend(); ++it)
        m_tokens.insert(it.key().trimmed().toLower(), it.value());
}

Token StateColors::token(const QString &state) const
{
    return m_tokens.value(state.trimmed().toLower(), m_default);
}

QColor StateColors::color(const QString &state) const
{
    return duifx::color(token(state));
}

QString StateColors::hex(const QString &state) const
{
    return colorHex(token(state));
}

QString StateColors::get(const QString &state, const QString &defaultHex) const
{
    if (!contains(state) && !defaultHex.isNull())
        return defaultHex;
    return hex(state);
}

bool StateColors::contains(const QString &state) const
{
    return m_tokens.contains(state.trimmed().toLower());
}

const StateColors &stateColors()
{
    static const StateColors instance;
    return instance;
}

} // namespace duifx
