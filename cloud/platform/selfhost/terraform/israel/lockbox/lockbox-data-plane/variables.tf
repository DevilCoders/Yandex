variable "endpoint" {
  default = "dpl.lockbox.crypto.yandexcloud.co.il"
}

variable "private_endpoint" {
  default = "lockbox.crypto.yandexcloud.co.il"
}

variable "public_endpoint" {
  default = "dpl.lockbox.api.cloudil.com"
}

variable "endpoint_name" {
  # endpoint minus dns zone
  default = "dpl"
}

variable "public_endpoint_name" {
  # endpoint minus dns zone
  default = "public-dpl"
}

variable "hostname_prefix" {
  default = "dpl"
}

variable "role_label" {
  default = "lockbox-data"
}

variable "default_kms_key_id" {
  default = "yc.lockbox.default-key"
}

variable "id_prefix" {
  # Taken from https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/endpoint_ids/ids-israel.yaml
  default = "bcn"
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
  default = "1500Mi"
}

variable "java_heap_memory" {
  default = "1000m"
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
