# Configuration Reference

Open `Configuration → Builtins → GPU` inside GKrellM to access plugin settings.

## Text Overlay
- Enable/disable the text overlay via the checkbox.
- Edit the text format string in the entry box. The default is `\D2$V\D0\t\f$G`.

Supported tokens:
- `\D0`, `\D1`, `\D2` — switch to GKrellM data layers for draw-order control.
- `$g`, `$v` — numeric GPU and VRAM utilisation percentages.
- `$G`, `$V` — labelled strings: `GPU <x>%`, `VRAM <y>%`.
- `\t` — horizontal tab; `\n` — newline.
- `\f` — reset to default font size; combine with `\s` for small text.

Any unrecognised escape sequence is copied literally.

## Chart Options
- Colours and gradients are derived from the GKrellM theme style `gpu`. Adjust via theme editor if desired.
- VRAM data is stored in chart channel 1; GPU utilisation is channel 0. Use GKrellM's chart configuration dialogue for advanced styling.

## Persistence
Settings are stored in GKrellM's `~/.gkrellm2/user_config` file under the `gpu` section. Deleting the section resets the plugin to defaults on next launch.
