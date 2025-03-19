{{- with .Values.pushClient }}
---
{{- if .ident }}
ident: {{ .ident }}
{{- end }}
topic: {{ required "You must specify topic for push-client" .topic }}

network:
  advice: {{ .dc }}
  master-addr: {{ .host }}
{{- if .database.enabled }}
  database: {{ .database.path }}
{{- end }}
{{ if .proto | eq "pq" }}
  proto: pq
{{- if .masterPort.enabled }}
  master-port: 2135
{{- end }}
{{- if .ssl.enabled }}
  ssl:
    enabled: 1
{{- end }}
{{- if .tvm.enabled }}
  tvm-client-id: {{ .tvm.clientId }}
  tvm-server-id: {{ .tvm.serverId }}
  tvm-secret-file: {{ .tvm.secretFile }}
{{- else if .oauth.enabled }}
  oauth-secret-file: {{ .oauth.secretFile }}
{{- else if .iam.enabled }}
  iam: 1
  iam-key-file: {{ .iam.keyFile }}
  iam-endpoint: {{ .iam.endpoint }}
{{- else }}
{{- fail "You must enable and configure one of authetication methods: tvm, oauth or iam" -}}
{{- end }}
{{- else }}
  proto: rt
{{- end }}
{{- end }}

logger:
  mode: stderr
  telemetry_interval: -1
  remote: 0
  level: 6

watcher:
  state: /var/lib/push-client/{{ $.Release.Namespace }}-{{ include "yc-infra-pod.servicePodName" $ }}

files:
  - name: /var/log/fluent/{{ $.Release.Namespace }}-{{ include "yc-infra-pod.servicePodName" $ }}.*.log
