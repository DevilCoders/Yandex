variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-kms/common
  default = "bn3v39j1bo0qgd0d4emb"
}

variable "service_account_id" {
  // yc-kms/common/sa-kms
  default = "d3bic0s14n0mrnn86tmu"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "1"
}

variable "ipv6_addresses" {
  // GPN: 2a0d:d6c0:200:200::7:*
  default = ["2a0d:d6c0:200:200::7:251"]
}

variable "ipv4_addresses" {
  // GPN: 172.16.0.*
  default = ["172.16.0.251"]
}

variable "subnets" {
  type = "map"

  default = {
    "ru-gpn-spb99" = "e57ope2brmae22de3r3v"
  }
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
  default     = "network-hdd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
