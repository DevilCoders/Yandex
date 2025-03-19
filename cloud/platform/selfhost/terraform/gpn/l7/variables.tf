variable "cloud_id" {
  default = "bn3rfvtr44kn892nnpim" # yc-cloud-platform-l7
}

variable "folder_id" {
  default = "bn3v8a9sj6u8ih3a088r" # common
}

variable "gpn_sa_file" {
  # sec-01ecmcrsn5t7jr0f6ks8hz8p1c
  default = ""
}

locals {
  gpn_sa_file = (var.gpn_sa_file != "" ? var.gpn_sa_file : pathexpand("~/sa-key/l7.json"))
}

variable "crt_token" {
  # https://nda.ya.ru/t/G0Xt5TGm3W4VEo
  default = ""
}

locals {
  image_id = "d8oqaljdp26lt8maa51l" # CLOUD-53121, 09.09.2020

  api = {
    alb_lb = "albu97e2a18hqq439aia"

    hosts = join(",", [
      "api.gpn.yandexcloud.net",
      "*.api.gpn.yandexcloud.net",
      "console.gpn.yandexcloud.net",
      "serialws.gpn.yandexcloud.net",
      "cr.gpn.yandexcloud.net",
    ])
    hosts_2 = join(",", [
      "api.yac.techpark.local",
      "*.api.yac.techpark.local",
      "console.yac.techpark.local",
      "serialws.yac.techpark.local",
      "cr.yac.techpark.local",
    ])
  }

  cpl = {
    alb_lb = "fkecf45f0kp6p4e7id2a"

    hosts = join(",", [
      "*.private-api.ycp.gpn.yandexcloud.net",
      "solomon.ycp.gpn.yandexcloud.net",
      "monitoring-charts.ycp.gpn.yandexcloud.net",
      "*.mk8s-masters.private-api.ycp.gpn.yandexcloud.net"
    ])
  }

  staging = {
    alb_lb = "fke1f2qjo6s8upu89g8k"
  }

  xds = {
    endpoints = [
      "2a0d:d6c0:200:200::5:30",
      "2a0d:d6c0:200:200::5:31",
      "2a0d:d6c0:200:200::5:32",
    ]
    sni = "xds.gpn.yandexcloud.net"
  }

  net = {
    network_id = "cgmnqc1tl2gmc8qeclj8"
    subnet_ids = ["e57mq3uf1nimamcnoahu"]
    zones      = ["ru-gpn-spb99"]
  }
}
