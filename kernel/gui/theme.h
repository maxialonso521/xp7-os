#ifndef GUI_THEME_H
#define GUI_THEME_H

// ═══════════════════════════════════════════════════════════
//   XP7 OS — Tema Visual: XP Luna Blue + W7 Glass touches
// ═══════════════════════════════════════════════════════════

// ─── Colores del desktop ─────────────────────────────────────
#define THEME_DESKTOP_TOP    0x1F6B49   // Verde-teal XP (arriba)
#define THEME_DESKTOP_BOT    0x0D4A2E   // Verde-teal XP (abajo)

// ─── Barra de título (activa) — gradiente azul XP ────────────
#define THEME_TITLE_ACT_L    0x0A246A   // Azul oscuro (izquierda)
#define THEME_TITLE_ACT_R    0x3D89CC   // Azul claro  (derecha)
#define THEME_TITLE_ACT_TXT  0xFFFFFF   // Texto blanco

// ─── Barra de título (inactiva) — gris-azulado ───────────────
#define THEME_TITLE_INACT_L  0x7A8DC8   // Gris-azul (izquierda)
#define THEME_TITLE_INACT_R  0xA9BCE8   // Gris-azul claro
#define THEME_TITLE_INACT_TXT 0xD4D4D4  // Texto gris claro

// ─── Borde de ventana ─────────────────────────────────────────
#define THEME_WIN_BORDER_ACT   0x0A246A  // Activa: azul oscuro
#define THEME_WIN_BORDER_INACT 0x7A8DC8  // Inactiva: gris-azul
#define THEME_WIN_CONTENT_BG   0xFFFFFF  // Fondo del contenido

// ─── Botones de ventana ───────────────────────────────────────
#define THEME_BTN_CLOSE_BG     0xC81024  // Rojo XP
#define THEME_BTN_CLOSE_HOVER  0xFF3040  // Rojo brillante al hover
#define THEME_BTN_MIN_BG       0x4080C0  // Azul medio
#define THEME_BTN_MIN_HOVER    0x60A0E0
#define THEME_BTN_ICON_CLR     0xFFFFFF  // Iconos blancos

// ─── Taskbar ──────────────────────────────────────────────────
#define THEME_TASKBAR_BG_TOP   0x2060CC  // Azul XP (arriba)
#define THEME_TASKBAR_BG_BOT   0x0A3478  // Azul XP (abajo)
#define THEME_TASKBAR_H        36        // Altura en píxeles
#define THEME_TASKBAR_SEP      0x1040A0  // Separador

// ─── Botón Start ─────────────────────────────────────────────
#define THEME_START_BG_L       0x3A7E24  // Verde oscuro
#define THEME_START_BG_R       0x6AC828  // Verde brillante
#define THEME_START_W          80        // Ancho del botón
#define THEME_START_TXT        0xFFFFFF  // Texto blanco

// ─── Botones genéricos (estilo XP 3D) ────────────────────────
#define THEME_BTN_FACE         0xECE9D8  // Beige/crema XP
#define THEME_BTN_HIGHLIGHT    0xFFFFFF  // Borde superior
#define THEME_BTN_SHADOW       0xACA899  // Borde inferior
#define THEME_BTN_TEXT         0x000000  // Texto negro

// ─── Colores de texto en la terminal ─────────────────────────
#define THEME_TERM_BG          0x0C0C0C  // Casi negro
#define THEME_TERM_TEXT        0xC0C0C0  // Gris claro
#define THEME_TERM_PROMPT      0x3ECCF0  // Cyan brillante
#define THEME_TERM_ERROR       0xFF4444  // Rojo error
#define THEME_TERM_SUCCESS     0x44FF44  // Verde éxito

// ─── Dimensiones estándar ─────────────────────────────────────
#define THEME_TITLEBAR_H   22   // Altura de la barra de título
#define THEME_BORDER_W      2   // Ancho del borde de ventana
#define THEME_WINCAP_BTN_W 16   // Ancho botones close/min
#define THEME_WINCAP_BTN_H 14   // Alto botones close/min
#define THEME_WINCAP_PAD    4   // Padding entre botones

// ─── Colores del sistema (notificaciones, etc.) ──────────────
#define THEME_TOOLTIP_BG   0xFFFFCC  // Amarillo tooltip XP
#define THEME_TOOLTIP_TXT  0x000000
#define THEME_TOOLTIP_BRD  0x000000
#define THEME_SELECTION    0x316AC5  // Azul selección XP
#define THEME_SELECTION_TXT 0xFFFFFF

#endif // GUI_THEME_H
