---
name: O Último Turno
colors:
  surface: '#1d100e'
  surface-dim: '#1d100e'
  surface-bright: '#463532'
  surface-container-lowest: '#170b09'
  surface-container-low: '#261815'
  surface-container: '#2a1c19'
  surface-container-high: '#352623'
  surface-container-highest: '#41312e'
  on-surface: '#f7ddd8'
  on-surface-variant: '#e1bfb8'
  inverse-surface: '#f7ddd8'
  inverse-on-surface: '#3c2d2a'
  outline: '#a98a83'
  outline-variant: '#59413c'
  surface-tint: '#ffb4a5'
  primary: '#ffb4a5'
  on-primary: '#650b00'
  primary-container: '#c23b22'
  on-primary-container: '#ffe8e4'
  inverse-primary: '#b02e16'
  secondary: '#c9c6bf'
  on-secondary: '#31302c'
  secondary-container: '#484741'
  on-secondary-container: '#b8b5ae'
  tertiary: '#94ccff'
  on-tertiary: '#003352'
  tertiary-container: '#0071ad'
  on-tertiary-container: '#e2efff'
  error: '#ffb4ab'
  on-error: '#690005'
  error-container: '#93000a'
  on-error-container: '#ffdad6'
  primary-fixed: '#ffdad3'
  primary-fixed-dim: '#ffb4a5'
  on-primary-fixed: '#3e0400'
  on-primary-fixed-variant: '#8e1400'
  secondary-fixed: '#e6e2db'
  secondary-fixed-dim: '#c9c6bf'
  on-secondary-fixed: '#1c1c17'
  on-secondary-fixed-variant: '#484741'
  tertiary-fixed: '#cde5ff'
  tertiary-fixed-dim: '#94ccff'
  on-tertiary-fixed: '#001d32'
  on-tertiary-fixed-variant: '#004b74'
  background: '#1d100e'
  on-background: '#f7ddd8'
  surface-variant: '#41312e'
  carvao: '#2B2B28'
  tinta: '#1B1B18'
  fumaca: '#3A3934'
  papel: '#D8CFBC'
  osso: '#F0EADF'
  poeira: '#8A8175'
  tijolo: '#C23B22'
  oxido: '#A8321D'
  rubor: rgba(194,59,34,0.12)
typography:
  headline-lg:
    fontFamily: Playfair Display
    fontSize: 36px
    fontWeight: '900'
    lineHeight: 44px
    letterSpacing: -0.02em
  headline-md:
    fontFamily: Playfair Display
    fontSize: 28px
    fontWeight: '700'
    lineHeight: 36px
    letterSpacing: -0.02em
  headline-sm:
    fontFamily: Playfair Display
    fontSize: 22px
    fontWeight: '700'
    lineHeight: 28px
    letterSpacing: -0.02em
  body-lg:
    fontFamily: Courier Prime
    fontSize: 16px
    fontWeight: '400'
    lineHeight: 24px
  body-sm:
    fontFamily: Courier Prime
    fontSize: 14px
    fontWeight: '400'
    lineHeight: 20px
  label-lg:
    fontFamily: Courier Prime
    fontSize: 13px
    fontWeight: '700'
    lineHeight: 16px
    letterSpacing: 0.12em
  label-sm:
    fontFamily: Courier Prime
    fontSize: 11px
    fontWeight: '700'
    lineHeight: 14px
    letterSpacing: 0.12em
  mono-data:
    fontFamily: Courier Prime
    fontSize: 13px
    fontWeight: '400'
    lineHeight: 16px
    letterSpacing: 0.05em
rounded:
  sm: 0.125rem
  DEFAULT: 0.25rem
  md: 0.375rem
  lg: 0.5rem
  xl: 0.75rem
  full: 9999px
spacing:
  gutter: 24px
  margin: 96px
  space-xs: 4px
  space-sm: 8px
  space-md: 16px
  space-lg: 24px
  space-xl: 48px
---

# Design System: O Último Turno
## 1. Visual Theme & Atmosphere
A 1930s industrial noir investigation. The interface feels like a declassified
case file laid over a dark factory floor: charcoal surfaces, aged paper type,
ink-black dialogue panels, and a single brick-red accent that marks danger,
selection, and contradiction. The mood is cinematic but restrained. Density is
airy (4/10), variance is asymmetric (7/10), and motion is deliberate (5/10).
No neon, no glossy gradients, no playful rounded web cards. The player should
feel like they are reading a case file by a single hanging lamp.
The style guide board may use dark rounded cards for swatches and type
specimens. In-game screens use sharper geometry: 2–4px radii, cut corners,
1px borders, and strong negative space.
## 2. Color Palette & Roles
- **Carvão da Fábrica (#2B2B28)** — Main canvas and full-screen background.
- **Tinta de Imprensa (#1B1B18)** — Dialogue box, modal overlay, letterbox, deep shadow.
- **Fumaça (#3A3934)** — Raised panels, notebook rail, cards.
- **Papel Envelhecido (#D8CFBC)** — Primary body text on dark, light surfaces, paper labels.
- **Osso (#F0EADF)** — Headlines, high-emphasis text, primary button labels.
- **Poeira (#8A8175)** — Secondary text, hairlines, inactive icons, metadata.
- **Tijolo (#C23B22)** — The single accent: CTAs, selected states, focus rings, contradiction markers, key evidence.
- **Óxido (#A8321D)** — Pressed/darker red, small text on red, red borders on cream.
- **Rubor (rgba(194,59,34,0.12))** — Error wash, hover tint, contradiction background flash.
No pure black (#000000) and no pure white (#FFFFFF). No second accent hue.
Red is a signal, not a background. Never use small red text on charcoal.
## 3. Typography Rules
- **Display:** Playfair Display 700/900. Track-tight (-0.02em), controlled scale.
- **Body:** Courier Prime 400. Relaxed 1.5 leading, max 65ch, Papel or Osso on dark.
- **Labels:** Courier Prime 700, uppercase, 0.12em tracking, Poeira or Osso.
- **Mono/Data:** Courier Prime. Scores, timers, IDs, evidence codes.
- **Banned:** Inter, Arial, Times New Roman, Georgia, Garamond, Palatino.
- **Hierarchy:** weight + color + spacing, not just size. Never use more than
  three type sizes on one screen.
## 4. Component Stylings
- **Dialogue box:** Tinta at 94% opacity, 2px Poeira border, 4px Tijolo top
  accent, 2px radius with one cut corner. Nameplate above-left in Tijolo with
  Osso text. Body in Osso. Speaker portrait anchored right, never covering text.
- **Buttons:** Flat, 1px Poeira border, transparent or Fumaça fill. Primary action
  uses Tijolo fill with Osso label; pressed uses Óxido and -1px translate.
  Focus uses a 2px Tijolo outline offset 3px. No glow, no pill shape.
- **Choice buttons:** Left-aligned text, 56px tall, 1px border, red left bar on
  hover/selection. Use text, not icons, for interrogation choices.
- **Suspect roster:** Asymmetric 2-column list with portraits; selected suspect
  gets a 2px Tijolo border and a small red evidence tag. Avoid three equal cards.
- **Notebook:** Left rail with section tabs; active tab has a 3px Tijolo bar.
  Content uses thin Poeira dividers. Contradictions use a Rubor background and
  a Tijolo marker, never blinking text.
- **Reaction icons:** Line icons in Osso/Poeira; agree/disagree/neutral. Red only
  for contradiction or danger.
- **Inputs/Search:** Tinta fill, Poeira border, label above, red focus ring.
- **Loaders:** Skeleton lines matching final layout. No circular spinner.
- **Empty states:** Typewriter sentence in Poeira, e.g. "Nenhum depoimento
  registrado ainda." No generic "No data".
## 5. Layout Principles
- Master canvas: 1920×1080, minimum 1280×720.
- Safe margin: 96px at 1080p; 64px at 720p.
- 12-column grid, 24px gutters.
- Dialogue box occupies the bottom 26–30% of the screen.
- Portrait occupies 35–45% of screen height, anchored bottom-right or bottom-left.
- Menu: asymmetric split, title left, vertical actions left, atmospheric art right.
- Notebook: 30/70 split between rail and content.
- Trial: suspect list left, selected portrait/evidence right.
- No centered hero. No overlapping text. No horizontal scroll.
## 6. Motion & Interaction
- Typewriter reveal for dialogue: 35–45 chars/s, click/Enter skips to full line.
- Screen transition: 220ms fade through Tinta; no slidey web transitions.
- Contradiction: 120ms Rubor flash + 1px screen shake, then red marker persists.
- Portrait change: 180ms opacity/translate; no bounce.
- Hover: 120ms border/color shift. Pressed: 80ms.
- Static film grain and vignette on the background layer only.
- Animate transform/opacity only; no layout-thrashing motion.
## 7. Anti-Patterns (Banned)
- No pure black or pure white.
- No neon glow, purple, cyan, or gradient text.
- No Inter, Arial, Times, Georgia, Garamond, or Palatino.
- No emojis.
- No three equal feature cards.
- No centered hero/menu layout.
- No rounded pill buttons everywhere.
- No generic "Lorem ipsum", "John Doe", "Acme".
- No copied Persona assets or UI. Inspiration only: angular panels, strong
  nameplates, portrait-and-dialogue composition.
- No visual spoilers: the killer must not have a uniquely evil portrait.
- No text over faces or over the dialogue panel edge.