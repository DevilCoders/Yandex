variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  // yc-cloud-trail/default
  default = "aoef3bqr00fuvnfaufbv"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1"
}

variable "subnets_addresses" {
  type = "map"

  default = {
    "buc2ob908hpbecsc16ue" = "2a02:6b8:c0e:501:0:f81f:0:101"
    "bltvvmg3lg3ahhc0urat" = "2a02:6b8:c02:901:0:f81f:0:101"
    "fo2bnfaqlm244rpgs1og" = "2a02:6b8:c03:501:0:f81f:0:101"
  }
}
