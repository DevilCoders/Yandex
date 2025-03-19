variable "hostname_prefix" {
  default = "tool"
}

variable "role_label" {
  default = "kms-tool"
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
  default     = "2"
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

variable "id_prefix" {
  // Taken from https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/endpoint_ids/ids-israel.yaml
  default = "c42"
}
