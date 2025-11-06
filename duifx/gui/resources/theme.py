from dataclasses import dataclass
from typing import Dict

@dataclass
class GrayScale:
    """Gray shades from very light to very dark"""
    gray_50: str = "#f9fafb"    # Very light
    gray_100: str = "#f3f4f6"   # Very light
    gray_200: str = "#e5e7eb"   # Light
    gray_300: str = "#d1d5db"   # Light-medium
    gray_400: str = "#9ca3af"   # Medium
    gray_500: str = "#6b7280"   # Medium-dark
    gray_600: str = "#4b5563"   # Dark
    gray_700: str = "#374151"   # Very dark
    gray_800: str = "#1f2937"   # Very dark
    gray_900: str = "#111827"   # Extremely dark

@dataclass
class BlueScale:
    """Blue color gradations"""
    blue_50: str = "#eff6ff"    # Very light
    blue_100: str = "#dbeafe"   # Light
    blue_200: str = "#bfdbfe"   # Light-medium
    blue_300: str = "#93c5fd"   # Medium-light
    blue_400: str = "#60a5fa"   # Medium
    blue_500: str = "#3b82f6"   # Standard blue
    blue_600: str = "#2563eb"   # Medium-dark
    blue_700: str = "#1d4ed8"   # Dark
    blue_800: str = "#1e40af"   # Very dark
    blue_900: str = "#1e3a8a"   # Extremely dark

@dataclass
class GreenScale:
    """Green color gradations"""
    green_50: str = "#f0fdf4"   # Very light
    green_100: str = "#dcfce7"  # Light
    green_200: str = "#bbf7d0"  # Light-medium
    green_300: str = "#86efac"  # Medium-light
    green_400: str = "#4ade80"  # Medium
    green_500: str = "#22c55e"  # Standard green
    green_600: str = "#16a34a"  # Medium-dark
    green_700: str = "#15803d"  # Dark
    green_800: str = "#166534"  # Very dark
    green_900: str = "#14532d"  # Extremely dark

@dataclass
class RedScale:
    """Red color gradations"""
    red_50: str = "#fef2f2"     # Very light
    red_100: str = "#fee2e2"    # Light
    red_200: str = "#fecaca"    # Light-medium
    red_300: str = "#fca5a5"    # Medium-light
    red_400: str = "#f87171"    # Medium
    red_500: str = "#ef4444"    # Standard red
    red_600: str = "#dc2626"    # Medium-dark
    red_700: str = "#b91c1c"    # Dark
    red_800: str = "#991b1b"    # Very dark
    red_900: str = "#7f1d1d"    # Extremely dark

@dataclass
class YellowScale:
    """Yellow color gradations"""
    yellow_50: str = "#fefce8"  # Very light
    yellow_100: str = "#fef3c7" # Light
    yellow_200: str = "#fde68a" # Light-medium
    yellow_300: str = "#fcd34d" # Medium-light
    yellow_400: str = "#fbbf24" # Medium
    yellow_500: str = "#f59e0b" # Standard yellow
    yellow_600: str = "#d97706" # Medium-dark
    yellow_700: str = "#b45309" # Dark
    yellow_800: str = "#92400e" # Very dark
    yellow_900: str = "#78350f" # Extremely dark

@dataclass
class PurpleScale:
    """Purple color gradations"""
    purple_50: str = "#faf5ff"  # Very light
    purple_100: str = "#f3e8ff" # Light
    purple_200: str = "#e9d5ff" # Light-medium
    purple_300: str = "#d8b4fe" # Medium-light
    purple_400: str = "#c084fc" # Medium
    purple_500: str = "#a855f7" # Standard purple
    purple_600: str = "#9333ea" # Medium-dark
    purple_700: str = "#7c3aed" # Dark
    purple_800: str = "#6b21a8" # Very dark
    purple_900: str = "#581c87" # Extremely dark

@dataclass
class OrangeScale:
    """Orange color gradations"""
    orange_50: str = "#fff7ed"  # Very light
    orange_100: str = "#ffedd5" # Light
    orange_200: str = "#fed7aa" # Light-medium
    orange_300: str = "#fdba74" # Medium-light
    orange_400: str = "#fb923c" # Medium
    orange_500: str = "#f97316" # Standard orange
    orange_600: str = "#ea580c" # Medium-dark
    orange_700: str = "#c2410c" # Dark
    orange_800: str = "#9a3412" # Very dark
    orange_900: str = "#7c2d12" # Extremely dark

@dataclass
class TealScale:
    """Teal color gradations"""
    teal_50: str = "#f0fdfa"    # Very light
    teal_100: str = "#ccfbf1"   # Light
    teal_200: str = "#99f6e4"   # Light-medium
    teal_300: str = "#5eead4"   # Medium-light
    teal_400: str = "#2dd4bf"   # Medium
    teal_500: str = "#14b8a6"   # Standard teal
    teal_600: str = "#0d9488"   # Medium-dark
    teal_700: str = "#0f766e"   # Dark
    teal_800: str = "#115e59"   # Very dark
    teal_900: str = "#134e4a"   # Extremely dark

class CompletePalette:
    """Complete color palette with all gradations"""
    
    # Gray scales
    gray = GrayScale()
    
    # Primary colors
    blue = BlueScale()
    green = GreenScale()
    red = RedScale()
    yellow = YellowScale()
    purple = PurpleScale()
    orange = OrangeScale()
    teal = TealScale()
    
    # Special colors
    white: str = "#ffffff"
    black: str = "#000000"
    
    # Semantic colors (based on scales)
    primary = blue.blue_500
    secondary = gray.gray_500
    success = green.green_500
    warning = yellow.yellow_500
    danger = red.red_500
    info = teal.teal_500
    
    @classmethod
    def get_color_dict(cls) -> Dict[str, str]:
        """All colors as dictionary for template substitution"""
        colors = {}
        
        # Add gray scales
        for attr, value in cls.gray.__dict__.items():
            colors[attr] = value
            
        # Add all color scales
        scales = ['blue', 'green', 'red', 'yellow', 'purple', 'orange', 'teal']
        for scale_name in scales:
            scale = getattr(cls, scale_name)
            for attr, value in scale.__dict__.items():
                colors[attr] = value
        
        # Special colors
        colors['white'] = cls.white
        colors['black'] = cls.black
        colors['primary'] = cls.primary
        colors['secondary'] = cls.secondary
        colors['success'] = cls.success
        colors['warning'] = cls.warning
        colors['danger'] = cls.danger
        colors['info'] = cls.info
        
        return colors

# Palette instance for easy usage
palette = CompletePalette()