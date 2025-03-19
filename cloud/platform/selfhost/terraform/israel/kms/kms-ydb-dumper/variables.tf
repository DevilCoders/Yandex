variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  default = "yc.kms.service-folder"
}

variable "service_account_id" {
  default = "yc.kms.kms-sa"
}

variable "hostname_prefix" {
  default = "ydb-dumper"
}

variable "role_label" {
  default = "kms-ydb-dumper"
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
  default     = "100"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-hdd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
