{{/*
Expand the name of the chart.
*/}}
{{- define "logbroker.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Create a default fully qualified app name.
We truncate at 63 chars because some Kubernetes name fields are limited to this (by the DNS naming spec).
If release name contains chart name it will be used as a full name.
*/}}
{{- define "logbroker.fullname" -}}
{{- if .Values.fullnameOverride }}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- $name := default .Chart.Name .Values.nameOverride }}
{{- if contains $name .Release.Name }}
{{- .Release.Name | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" }}
{{- end }}
{{- end }}
{{- end }}

{{/*
Create chart name and version as used by the chart label.
*/}}
{{- define "logbroker.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Common labels
*/}}
{{- define "logbroker.labels" -}}
helm.sh/chart: {{ include "logbroker.chart" . }}
{{ include "logbroker.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/*
Selector labels
*/}}
{{- define "logbroker.selectorLabels" -}}
app.kubernetes.io/name: {{ include "logbroker.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
Create the name of the service account to use
*/}}
{{- define "logbroker.serviceAccountName" -}}
{{- if .Values.serviceAccount.create }}
{{- default (include "logbroker.fullname" .) .Values.serviceAccount.name }}
{{- else }}
{{- default "default" .Values.serviceAccount.name }}
{{- end }}
{{- end }}

{{/*
Create the name of the namespace to use
*/}}
{{- define "logbroker.namespace" -}}
{{- default (include "logbroker.fullname" .) .Values.namespace.name }}
{{- end }}


{{/*
Create topics list
*/}}
{{- define "logbroker.topics" -}}
{{- join "," .Values.topics }}
{{- end }}

{{/*
Create partitions value
*/}}
{{- define "logbroker.partitions" -}}
{{- printf "\"%.f\"" .Values.partitions }}
{{- end }}

{{/*
Create grpc env port
*/}}
{{- define "logbroker.grpcPort" -}}
{{- printf "\"%.f\"" .Values.service.port }}
{{- end }}



{{/*
Create migration name
*/}}
{{- define "logbroker.migrationName" -}}
{{- printf "migrate-%s" (include "logbroker.chart" .) }}
{{- end }}
