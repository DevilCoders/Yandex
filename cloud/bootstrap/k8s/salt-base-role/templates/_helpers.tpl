{{/*
Expand the name of the chart.
*/}}
{{- define "chart.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Create a default fully qualified app name.
We truncate at 63 chars because some Kubernetes name fields are limited to this (by the DNS naming spec).
If release name contains chart name it will be used as a full name.
*/}}
{{- define "chart.fullname" -}}
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
{{- define "chart.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Common labels
*/}}
{{- define "chart.labels" -}}
helm.sh/chart: {{ include "chart.chart" . }}
{{ include "chart.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/*
Selector labels
*/}}
{{- define "chart.selectorLabels" -}}
app.kubernetes.io/name: {{ include "chart.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
Create the name of the service account to use
*/}}
{{- define "chart.serviceAccountName" -}}
{{- if .Values.serviceAccount.create }}
{{- default (include "chart.fullname" .) .Values.serviceAccount.name }}
{{- else }}
{{- default "default" .Values.serviceAccount.name }}
{{- end }}
{{- end }}

{{/*
Create saltformula resource
*/}}
{{- define "chart.saltformula" -}}
{{- $roleName := .Template.Name | base | trimSuffix ".yaml" }}
{{- $role := get .Values.roles $roleName }}
{{- if $role }}
---
apiVersion: bootstrap.cloud.yandex.net/v1alpha1
kind: SaltFormula
metadata:
  name: {{ $roleName | replace "_" "-" }}
  namespace: {{ .Release.Namespace }}
spec:
  baseRole: {{ .Values.baseRole }}
  role: {{ $roleName }}
  version: {{ $role.version | quote }}
  apply: {{ $role.apply | default "True" }}
  batchSize: {{ $role.batch_size | default 1 }}
{{- if $role.max_fail }}
  maxFail: {{ $role.max_fail }}
{{- end }}
{{- if $role.skip_lock_updated_hosts }}
  skipLockUpdatedHosts: true
{{- end }}
  nodeSelector:
{{- range $key, $value := .Values.nodeSelector }}
    {{ $key }}: {{ $value }}
{{- end }}
{{- end }}
{{- end }}
