variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-crypto-e2e-monitoring/benchmark
  default = "b1g2li1qkptm8kfo5g7k"
}

variable "service_account_id" {
  // yc-crypto-e2e-monitoring/benchmark
  default = "ajens7k0jvhhcjlqq5v8"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
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
