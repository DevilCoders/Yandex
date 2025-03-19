variable "endpoint" {
  default = "cpl.ycm.crypto.yandexcloud.co.il"
}

variable "private_endpoint" {
  default = "ycm.crypto.yandexcloud.co.il"
}

variable "public_endpoint" {
  default = "cpl.ycm.api.cloudil.com"
}

variable "endpoint_name" {
  # endpoint minus dns zone
  default = "cpl"
}

variable "public_endpoint_name" {
  # endpoint minus dns zone
  default = "public-cpl"
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
  # Taken from https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/endpoint_ids/ids-israel.yaml
  default = "b25"
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
  default = "750Mi"
}

variable "tool_heap_memory" {
  default = "600m"
}

variable "instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
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
  default     = "25"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
