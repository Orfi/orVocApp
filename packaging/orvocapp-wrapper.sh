#!/bin/sh
# Wrapper script for installed orVocApp — sets library, QML, plugin, and font
# paths so bundled Qt libs and system resources work together.
ORVOCAPP_LIB="/usr/lib/orvocapp"
export LD_LIBRARY_PATH="${ORVOCAPP_LIB}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${ORVOCAPP_LIB}/plugins"
export QML2_IMPORT_PATH="${ORVOCAPP_LIB}/qml"
export FONTCONFIG_PATH="/etc/fonts"
export QT_QPA_PLATFORMTHEME=none
cd "$HOME" 2>/dev/null || true
exec "${ORVOCAPP_LIB}/orVocApp" "$@"
