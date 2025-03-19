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

variable "network_id" {
  description = "Network id"
  default = "enpsm2k46k6n43vrk6fg"
}

variable "security_group_ids" {
  default = ["enp9o9k4rdsi0tlursh2"]
}

variable "subnets" {
  type = "map"

  default = {
    "ru-central1-a" = "e9beh1u93kbb9kkb340d"
    "ru-central1-b" = "e2l9ohqt4eaf1mre6puj"
    "ru-central1-c" = "b0cbr1gsb85d5dibp3e1"
  }
}

variable "subnets_addresses" {
  type = "map"

  // preparer
  default = {
    "e9beh1u93kbb9kkb340d" = "2a02:6b8:c0e:500:0:f837:ebf2:125"
    "e2l9ohqt4eaf1mre6puj" = "2a02:6b8:c02:900:0:f837:ebf2:125"
    "b0cbr1gsb85d5dibp3e1" = "2a02:6b8:c03:500:0:f837:ebf2:125"
  }
}
