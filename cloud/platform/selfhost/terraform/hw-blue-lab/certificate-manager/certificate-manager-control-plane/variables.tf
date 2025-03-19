variable "endpoint" {
  default = "cpl.cert-manager.hw-blue.cloud-lab.yandex.net"
}

variable "endpoint_name" {
  # endpoint minus dns zone
  default = "cpl"
}

variable "hostname_prefix" {
  default = "cpl"
}

variable "system_kms_key_id" {
  default = "yc.certificate-manager.certificates-key"
}

variable "role_label" {
  default = "certificate-manager-control"
}

variable "id_prefix" {
  # Copied from preprod/certificate-manager. No one cares if it is unique.
  default = "fd3"
}

variable "api_port" {
  default = "443"
}

variable "hc_port" {
  default = "444"
}

variable "private_api_port" {
  default = "8443"
}

variable "private_hc_port" {
  default = "8444"
}

variable "configserver_pod_memory" {
  default = "256Mi"
}

variable "gateway_pod_memory" {
  default = "128Mi"
}

variable "envoy_pod_memory" {
  default = "320Mi"
}

variable "envoy_heap_memory" {
  default = "268435456" # 256 Mb
}

variable "java_pod_memory" {
  default = "1250Mi"
}

variable "java_heap_memory" {
  default = "750m"
}

variable "tool_pod_memory" {
  default = "256Mi"
}

variable "tool_heap_memory" {
  default = "200m"
}

variable "instance_group_size" {
  description = "Number of instances to deploy"
  default     = "1"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "4"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "15"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
