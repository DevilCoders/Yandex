variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  default = "yc.schecker"
}

variable "network_id" {
  default = "c64au1th7jcvp270e7vo"
}

variable "service_account_id" {
  # yc.schecker/sa-schecker
  default = "bfbn01ngbhi7ilq1dvgg"
}

variable "yc_schecker_db_connection" {
  default = {
    host = "rc1c-i5gr2biafpi1ztwg.mdb.cloud-preprod.yandex.net"
    port = "6432"
    name = "schecker_db_preprod"
  }
}

variable "yc_schecker_conductor_url" {
  default = "https://c.yandex-team.ru/api"
}

variable "security_group_ids" {
  default = ["c640l3bvi4fciu7a5pe0"]
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
  # VLA: 2a02:6b8:c0e:501:0:fc5b:0:*
  # SAS: 2a02:6b8:c02:901:0:fc5b:0:*
  # MYT: 2a02:6b8:c03:501:0:fc5b:0:*
  default = ["2a02:6b8:c03:501:0:fc5b:0:101"]
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
    "ru-central1-a" = "buchutusm154aj2dssjr"
    "ru-central1-b" = "bltfvgqs1oj7n208qk9a"
    "ru-central1-c" = "fo2ice2rh6fm80q2ejok"
  }
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "2"
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

variable "metadata_image_version" {
  type    = string
  default = "82cc2cb932"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
