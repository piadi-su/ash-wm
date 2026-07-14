# Legge il valore iniziale
xprop -root -notype _ASHWM_WORKSPACES

# Ascolta in tempo reale i cambiamenti (senza consumare CPU!)
xprop -root -spy _ASHWM_WORKSPACES | while read -r line; do
    # 'line' conterrà qualcosa come: _ASHWM_WORKSPACES = "1:A 2:O 3:E 4:E..."
    # Qui puoi formattare l'output come piace a te per la barra!
    clean_status=$(echo "$line" | cut -d'"' -f2)
    echo "I miei Workspace: $clean_status"
done
