variable "endpoint" {
  default = "validation.ycm.crypto.yandexcloud.co.il"
}

variable "public_endpoint" {
  default = "validation.ycm.api.cloudil.com"
}

variable "hostname_prefix" {
  default = "validation"
}

variable "role_label" {
  default = "certificate-manager-validation"
}

variable "endpoint_name" {
  # endpoint minus dns zone
  default = "validation"
}

variable "public_endpoint_name" {
  # endpoint minus dns zone
  default = "public-validation"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "instance_core_fraction" {
  description = "Core fraction per one instance"
  default     = 20
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

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
