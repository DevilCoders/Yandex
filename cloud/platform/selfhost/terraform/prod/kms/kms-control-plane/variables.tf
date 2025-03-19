variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-kms/default
  default = "b1gbam1e0jacoa7bbkq2"
}

variable "service_account_id" {
  // yc-kms/sa-kms-prod
  default = "aje70ums535p6lc3hgoc"
}

variable "security_group_ids" {
  default = ["enpipbq0jitq8av9ntle"]
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:500:0:f830:23d8:*
  // SAS: 2a02:6b8:c02:900:0:f830:23d8:*
  // MYT: 2a02:6b8:c03:500:0:f830:23d8:*
  default = [
    "2a02:6b8:c0e:500:0:f830:23d8:100",
    "2a02:6b8:c02:900:0:f830:23d8:100",
    "2a02:6b8:c03:500:0:f830:23d8:100"
  ]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  default = [
    "172.16.0.50",
    "172.17.0.50",
    "172.18.0.50"
  ]
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "e9bu4i9ou1tsuuulrpch"
    "ru-central1-b" = "e2lrodo8qt7no7p1sauo"
    "ru-central1-c" = "b0c2jre602gl2ltl3mod"
  }
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
