variable "api_port" {
  default = 443
}

variable "instance_group_size" {
  default = 3
}

variable "hostname_prefix" {
  default     = "prefix"
  description = "Prefix to form instance hostname"
}

variable "hostname_suffix" {
  default     = "svc.cloud-preprod.yandex.net"
  description = "Suffix to form instance hostname"
}

variable "zones" {
  type = list

  default = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ]
}

variable "host_suffix_for_zone" {
  type = map

  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}
