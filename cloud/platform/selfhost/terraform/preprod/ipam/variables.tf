variable "address_count_produce" {
  default     = 10
  description = "Address count to generate for one service in one zone"
}

variable "yc_token" {
  description = "Yandex Cloud security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  default     = "aoe5k83dn6vak86d5a3i"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisoned resources"
  default     = "ru-central1-c"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud-preprod.yandex.net:443"
}

// TODO: make use variables instead static definition
variable "zone_subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "bucpba0hulgrkgpd58qp"
    "ru-central1-b" = "bltueujt22oqg5fod2se"
    "ru-central1-c" = "fo27jfhs8sfn4u51ak2s"
  }
}

variable "zone_suffix_list" {
  type = list(string)

  default = [
    "vla",
    "sas",
    "myt",
  ]
}

