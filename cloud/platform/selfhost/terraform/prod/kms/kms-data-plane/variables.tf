variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-kms/default
  default = "b1gbam1e0jacoa7bbkq2"
}

variable "service_account_id" {
  // yc-kms/sa-kms-prod
  default = "aje70ums535p6lc3hgoc"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "6"
}

variable "private_api_port" {
  description = "Must be the same as grpcServer port in application.yaml"
  default     = "9443"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "14"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "56"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "90"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
