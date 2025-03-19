variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  // yc-certificate-manager/default
  default     = "aoehdr81kgmuudc48h60"
}

variable "service_account_id" {
  // yc-certificate-manager/sa-certificate-manager-cpl
  default = "bfbdcklop8i6hn2qrr86"
}

variable "security_group_ids" {
  default = ["c64vis55i63lpb8j8i7e"]
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:501:0:f820:0:*
  // SAS: 2a02:6b8:c02:901:0:f820:0:*
  // MYT: 2a02:6b8:c03:501:0:f820:0:*
  // PREPROD IPs start with 100
  default = ["2a02:6b8:c0e:501:0:f820:0:101", "2a02:6b8:c02:901:0:f820:0:101", "2a02:6b8:c03:501:0:f820:0:101"]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  // PREPROD IPs start with 100
  default = ["172.16.0.101", "172.17.0.101", "172.18.0.101"]
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "bucmetc0o2sg10b9i971"
    "ru-central1-b" = "blt466adnf2pm52pdrgr"
    "ru-central1-c" = "fo2ldt67cnh4uskausvf"
  }
}

variable "subnets_ipv4_addresses" {
  type = map(string)

  default = {
    "bucmetc0o2sg10b9i971" = "172.16.0.101"
    "blt466adnf2pm52pdrgr" = "172.17.0.101"
    "fo2ldt67cnh4uskausvf" = "172.18.0.101"
  }
}

//variable "ipv4_address" {
//  type    = string
//  default = "130.193.32.212"
//}

variable "ipv6_address" {
  type    = string
  default = "2a0d:d6c0:0:ff1b::33e"
}

variable "subnets_ipv6_addresses" {
  type = map(string)

  default = {
    "bucmetc0o2sg10b9i971" = "2a02:6b8:c0e:501:0:f820:0:101"
    "blt466adnf2pm52pdrgr" = "2a02:6b8:c02:901:0:f820:0:101"
    "fo2ldt67cnh4uskausvf" = "2a02:6b8:c03:501:0:f820:0:101"
  }
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
  default     = "8"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "25"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-hdd"
}

variable "private_api_port" {
  description = "Must be the same as grpcServer port in application.yaml"
  default     = "9443"
}

variable "private_api_healthcheck_port" {
  description = "Must be the same as healthcheck port in application.yaml"
  default     = "9444"
}

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
