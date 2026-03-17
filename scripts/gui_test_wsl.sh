#!/usr/bin/env bash
set -euo pipefail
PROJECT="/mnt/c/Users/Vini_/OneDrive - Instituto Presbiteriano Mackenzie/Área de Trabalho/Vesta/Comp Visual/ProjetoCompVisu"
cd "$PROJECT"

IMAGE_PATH="${1:-}"
if [[ -z "$IMAGE_PATH" ]]; then
  echo "Uso: scripts/gui_test_wsl.sh caminho/para/imagem.png"
  exit 1
fi

if [[ ! -f "$IMAGE_PATH" ]]; then
  echo "Imagem nao encontrada: $IMAGE_PATH"
  exit 1
fi

export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:${PKG_CONFIG_PATH:-}
export LD_LIBRARY_PATH=/usr/local/lib:${LD_LIBRARY_PATH:-}

rm -f gui_fullscreen.png gui_main_window.png gui_secondary_window.png \
  gui_before_click.png gui_after_click1.png gui_after_click2.png gui_after_key_s.png \
  /tmp/gui_run.log /tmp/xvfb_proj.log

pkill -f "Xvfb :100" >/dev/null 2>&1 || true
Xvfb :100 -screen 0 1600x900x24 >/tmp/xvfb_proj.log 2>&1 &
XVFB_PID=$!
export DISPLAY=:100
sleep 1

./build/main "$IMAGE_PATH" >/tmp/gui_run.log 2>&1 &
APP_PID=$!

cleanup() {
  kill "$APP_PID" 2>/dev/null || true
  kill "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT

MAIN_WIN=""
SEC_WIN=""
for _ in $(seq 1 40); do
  MAIN_WIN=$(xdotool search --name "Processamento de Imagens - Principal" | head -n1 || true)
  SEC_WIN=$(xdotool search --name "Processamento de Imagens - Histograma" | head -n1 || true)
  if [[ -n "$MAIN_WIN" && -n "$SEC_WIN" ]]; then
    break
  fi
  sleep 0.25
done

if [[ -z "$MAIN_WIN" || -z "$SEC_WIN" ]]; then
  echo "GUI_WINDOWS_NOT_FOUND"
  tail -n 120 /tmp/gui_run.log || true
  exit 1
fi

echo "MAIN_WIN=$MAIN_WIN SEC_WIN=$SEC_WIN"

import -display :100 -window root gui_before_click.png

xdotool mousemove --window "$SEC_WIN" 200 450 click 1
sleep 0.8
import -display :100 -window root gui_after_click1.png
xdotool mousemove --window "$SEC_WIN" 200 450 click 1
sleep 0.8
import -display :100 -window root gui_after_click2.png

xdotool key --window "$MAIN_WIN" s
sleep 1
import -display :100 -window root gui_after_key_s.png

import -display :100 -window root gui_fullscreen.png
import -display :100 -window "$MAIN_WIN" gui_main_window.png || true
import -display :100 -window "$SEC_WIN" gui_secondary_window.png || true

xdotool windowclose "$SEC_WIN" || true
xdotool windowclose "$MAIN_WIN" || true
sleep 0.5

ls -lh gui_before_click.png gui_after_click1.png gui_after_click2.png gui_after_key_s.png \
  gui_fullscreen.png gui_main_window.png gui_secondary_window.png output_image.png
echo "--- GUI LOG TAIL ---"
tail -n 80 /tmp/gui_run.log || true
