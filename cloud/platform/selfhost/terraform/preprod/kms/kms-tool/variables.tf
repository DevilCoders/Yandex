variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  # yc-kms/default
  default = "aoea4pq3gmtisjam5u3j"
}

variable "service_account_id" {
  # yc-kms/sa-kms
  default = "bfbtj7imjcmd4712ojuq"
}

variable "security_group_ids" {
  default = ["c64hph4cil1hlp8838vb"]
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "1"
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:501:0:f810:0:*
  // SAS: 2a02:6b8:c02:901:0:f806:0:*
  // MYT: 2a02:6b8:c03:501:0:f810:0:*
  default = ["2a02:6b8:c0e:501:0:f810:0:251"]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  default = ["172.16.0.251"]
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "buctfrrbk16jvgum303f"
    "ru-central1-b" = "bltb1jtspj6kad29pnn3"
    "ru-central1-c" = "fo27i9oj1oqk41uaq660"
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
