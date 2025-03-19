variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  # yc-kms-test/default
  default = "aoelks3479sjaiart1pi"
}

variable "service_account_id" {
  # yc-kms-test/sa-kms
  default = "bfbr51ij1bptetapfqcs"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "4"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "16"
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
