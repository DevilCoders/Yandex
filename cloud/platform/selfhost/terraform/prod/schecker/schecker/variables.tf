variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  default = "yc.schecker"
}

variable "network_id" {
  default = "enpu8dfrqr3n99fll8vh"
}

variable "service_account_id" {
  # yc.schecker/sa-schecker
  default = "ajek99hlet4qi9ea5c7d"
}

variable "yc_schecker_db_connection" {
  default = {
    host = "rc1a-ox1c4wdmiyb25y4b.mdb.yandexcloud.net,rc1b-xjg5o0soepksr3e7.mdb.yandexcloud.net,rc1c-pddehqaepc35tedk.mdb.yandexcloud.net"
    port = "6432"
    name = "schecker_db_prod"
  }
}

variable "yc_schecker_conductor_url" {
  default = "https://c.yandex-team.ru/api/"
}

variable "security_group_ids" {
  default = ["enp768i1rm1mfp0mjqvf"]
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "1"
}

variable "custom_yc_zones" {
  description = "Yandex Cloud Zones to deploy in"

  # MYT
  default = ["ru-central1-c"]
}

variable "ipv6_addresses" {
  # VLA: 2a02:6b8:c0e:500:0:0:f873:0:*
  # SAS: 2a02:6b8:c02:900:0:fc5b:0:*
  # MYT: 2a02:6b8:c03:500:0:fc5b:0:*

  default = ["2a02:6b8:c03:500:0:f873:0:101"]
}

variable "ipv4_addresses" {
  # VLA: 172.16.0.*
  # SAS: 172.17.0.*
  # MYT: 172.18.0.*
  default = ["172.18.0.101"]
}

variable "zones" {
  type = list
  default = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c"
  ]
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "e9bgrqgj1mnnsr7lq5df"
    "ru-central1-b" = "e2lvov76kq60i81g23r1"
    "ru-central1-c" = "b0c55g5nqv1h8bpj87fp"
  }
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "8"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "32"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "50"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

variable "metadata_image_version" {
  type    = string
  default = "82cc2cb932"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
