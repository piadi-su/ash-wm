#!/usr/bin/env bash

#test ipc scritp

xprop -root -notype _ASHWM_WORKSPACES

xprop -root -spy _ASHWM_WORKSPACES | while read -r line; do
    clean_status=$(echo "$line" | cut -d'"' -f2)
    echo "Workspace: $clean_status"
done
