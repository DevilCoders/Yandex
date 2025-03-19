{{- with .Values.jugglerClient }}
[client]
{{- if .checkBundles }}
check_bundles={{ .checkBundles }}
{{- end }}
config_url={{ .configUrl }}
targets={{ .targets }}
log_level=0
checks_discover_dirs={{ .checkDiscoverDirs }}
instance_port={{ required "You must specify instance port for juggler" .instancePort }}
{{- end }}
