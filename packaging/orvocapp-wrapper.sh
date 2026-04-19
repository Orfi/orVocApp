#!/bin/sh
# Wrapper script for installed orVocApp — sets library path so bundled Qt libs
# and their plugins can find each other.
ORVOCAPP_LIB="/usr/lib/orvocapp"
export LD_LIBRARY_PATH="${ORVOCAPP_LIB}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${ORVOCAPP_LIB}/plugins"
exec "${ORVOCAPP_LIB}/orVocApp" "$@"
