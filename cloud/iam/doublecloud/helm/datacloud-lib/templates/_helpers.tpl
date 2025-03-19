{{/*
Expand the name of the chart.
*/}}
{{- define "datacloud-lib.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Create chart name and version as used by the chart label.
*/}}
{{- define "datacloud-lib.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Common labels
*/}}
{{- define "datacloud-lib.labels" -}}
helm.sh/chart: {{ include "datacloud-lib.chart" . }}
{{ include "datacloud-lib.selectorLabels" . }}
app.kubernetes.io/name: {{ include "datacloud-lib.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/*
Selector labels
*/}}
{{- define "datacloud-lib.selectorLabels" -}}
{{- if .Values.serviceName }}
yc.iam.app: {{ .Values.serviceName }}
{{- end }}
{{ include "datacloud-lib.selectorEnvLabels" . }}
{{- end }}

{{/*
Service (NLB) selector labels
*/}}
{{- define "datacloud-lib.selectorEnvLabels" -}}
yc.iam.env: "iam"
{{- end }}

{{/*
Get path to directory with rendered config files of current environment (CSP and Service)
*/}}
{{- define "datacloud-lib.getConfigsPath" -}}
{{- printf "%s/%s/files" .Values.cloudServiceProvider .Values.serviceName }}
{{- end }}

{{/*
Extract environment variables from env file. And render deployment env
*/}}
{{- define "datacloud-lib.renderEnvironmentVariables" -}}
{{- $filename := printf "%s/config.env" (include "datacloud-lib.getConfigsPath" . ) }}
{{- $parts := (.Files.Get $filename | replace "\\\n" " " | replace "=\"" ": \"" | split "\n" ) }}
{{- range $parts }}
{{- if . }}
{{- $item := . | split ": " }}
- name: {{ $item._0 | replace "JAVA_OPTS" "_JAVA_OPTIONS" }}
  value: {{ regexReplaceAll "\\s+" $item._1 " \\\n          " }}
{{- end }}
{{- end }}
{{- end }}
