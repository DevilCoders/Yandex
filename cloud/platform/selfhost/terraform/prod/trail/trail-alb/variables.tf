variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  // yc-cloud-trail/default
  default = "b1gtrvd4ntcgpob8ga7r"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1"
}

variable "subnets_addresses" {
  type = "map"

  default = {
    "e9beh1u93kbb9kkb340d" = "2a02:6b8:c0e:500:0:f837:ebf2:101"
    "e2l9ohqt4eaf1mre6puj" = "2a02:6b8:c02:900:0:f837:ebf2:101"
    "b0cbr1gsb85d5dibp3e1" = "2a02:6b8:c03:500:0:f837:ebf2:101"
  }
}
