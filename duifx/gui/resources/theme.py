"""Color palette for DUIFx theming (Tailwind-inspired scales)."""

from dataclasses import dataclass, fields


@dataclass(frozen=True)
class ColorScale:
    """Ten-shade color scale from light (50) to dark (900)."""
    s50: str
    s100: str
    s200: str
    s300: str
    s400: str
    s500: str
    s600: str
    s700: str
    s800: str
    s900: str


class Palette:
    """Complete color palette with semantic aliases."""

    gray   = ColorScale("#f9fafb", "#f3f4f6", "#e5e7eb", "#d1d5db", "#9ca3af",
                        "#6b7280", "#4b5563", "#374151", "#1f2937", "#111827")
    blue   = ColorScale("#eff6ff", "#dbeafe", "#bfdbfe", "#93c5fd", "#60a5fa",
                        "#3b82f6", "#2563eb", "#1d4ed8", "#1e40af", "#1e3a8a")
    green  = ColorScale("#f0fdf4", "#dcfce7", "#bbf7d0", "#86efac", "#4ade80",
                        "#22c55e", "#16a34a", "#15803d", "#166534", "#14532d")
    red    = ColorScale("#fef2f2", "#fee2e2", "#fecaca", "#fca5a5", "#f87171",
                        "#ef4444", "#dc2626", "#b91c1c", "#991b1b", "#7f1d1d")
    yellow = ColorScale("#fefce8", "#fef3c7", "#fde68a", "#fcd34d", "#fbbf24",
                        "#f59e0b", "#d97706", "#b45309", "#92400e", "#78350f")
    purple = ColorScale("#faf5ff", "#f3e8ff", "#e9d5ff", "#d8b4fe", "#c084fc",
                        "#a855f7", "#9333ea", "#7c3aed", "#6b21a8", "#581c87")
    orange = ColorScale("#fff7ed", "#ffedd5", "#fed7aa", "#fdba74", "#fb923c",
                        "#f97316", "#ea580c", "#c2410c", "#9a3412", "#7c2d12")
    teal   = ColorScale("#f0fdfa", "#ccfbf1", "#99f6e4", "#5eead4", "#2dd4bf",
                        "#14b8a6", "#0d9488", "#0f766e", "#115e59", "#134e4a")

    white: str = "#ffffff"
    black: str = "#000000"

    # Semantic aliases
    primary   = blue.s500
    secondary = gray.s500
    success   = green.s500
    warning   = yellow.s500
    danger    = red.s500
    info      = teal.s500

    @classmethod
    def get_color_dict(cls) -> dict[str, str]:
        """All colors as flat dictionary (e.g. ``gray_50``, ``blue_500``, …)."""
        colors: dict[str, str] = {}
        scale_names = ("gray", "blue", "green", "red", "yellow", "purple", "orange", "teal")
        for name in scale_names:
            scale = getattr(cls, name)
            for f in fields(scale):
                colors[f"{name}_{f.name[1:]}"] = getattr(scale, f.name)
        colors.update(white=cls.white, black=cls.black, primary=cls.primary,
                      secondary=cls.secondary, success=cls.success, warning=cls.warning,
                      danger=cls.danger, info=cls.info)
        return colors


palette = Palette()