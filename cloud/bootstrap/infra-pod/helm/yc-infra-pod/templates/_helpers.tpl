{{/* vim: set filetype=mustache: */}}
{{/*
Expand the name of the chart.
*/}}
{{- define "yc-infra-pod.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{/*
Create a default fully qualified app name.
We truncate at 63 chars because some Kubernetes name fields are limited to this (by the DNS naming spec).
If release name contains chart name it will be used as a full name.
*/}}
{{- define "yc-infra-pod.fullname" -}}
{{- if .Values.fullnameOverride -}}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" -}}
{{- else -}}
{{- $name := default .Chart.Name .Values.nameOverride -}}
{{- if contains $name .Release.Name -}}
{{- .Release.Name | trunc 63 | trimSuffix "-" -}}
{{- else -}}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" -}}
{{- end -}}
{{- end -}}
{{- end -}}

{{/*
Create chart name and version as used by the chart label.
*/}}
{{- define "yc-infra-pod.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{/*
Common labels
*/}}
{{- define "yc-infra-pod.labels" -}}
app.kubernetes.io/name: {{ include "yc-infra-pod.name" . }}
helm.sh/chart: {{ include "yc-infra-pod.chart" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end -}}

{{/*
Create the name of the service pod to use
*/}}
{{- define "yc-infra-pod.servicePodName" -}}
{{ default .Release.Name .Values.servicePodName }}
{{- end -}}

{{/*
Creates secret volume mounts
*/}}
{{- define "yc-infra-pod.secretVolumeMounts" -}}
{{- range $id, $secret := .secrets }}
- name: {{ $.pathPrefix | sha1sum | trunc 7 }}-{{ $id }}
  mountPath: {{ $.pathPrefix }}/{{ $secret.name }}
{{- end }}
{{- end -}}

{{/*
Creates secret volumes
*/}}
{{- define "yc-infra-pod.secretVolumes" -}}
{{- range $id, $secret := .secrets }}
- name: {{ $.pathPrefix | sha1sum | trunc 7 }}-{{ $id }}
  hostPath:
    type: File
    path: /usr/share/yc-secrets/{{ $id }}/{{ $secret.name }}/{{ $secret.version }}/{{ $secret.name }}
{{- end }}
{{- end -}}
