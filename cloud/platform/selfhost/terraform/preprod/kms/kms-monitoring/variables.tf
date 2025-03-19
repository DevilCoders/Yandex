variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-kms/kms-monitoring
  default = "aoes0aam45aunbvsk93k"
}

variable "service_account_id" {
  // yc-kms/kms-monitoring/sa-kms-monitoring
  default = "bfbsu62b4d6pfjfj6b1m"
}

variable "custom_host_group" {
  default = "service"
}

variable "yc_instance_group_size" {
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
  default     = "20"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
