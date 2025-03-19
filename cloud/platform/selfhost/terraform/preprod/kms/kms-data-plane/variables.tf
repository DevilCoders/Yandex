variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  # yc-kms-devel/default
  default = "aoea4pq3gmtisjam5u3j"
}

variable "service_account_id" {
  # yc-kms-devel/sa-kms
  default = "bfbtj7imjcmd4712ojuq"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
}

variable "custom_yc_zones" {
  description = "Yandex Cloud Zones to deploy in"

  # VLA, MYT
  default = ["ru-central1-a", "ru-central1-c"]
}

variable "host_group" {
  default = "service"
}

variable "private_api_port" {
  description = "Must be the same as grpcServer port in application.yaml"
  default     = "9443"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "8"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "16"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "75"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
