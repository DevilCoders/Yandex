variable "yc_folder" {
  type        = string
  description = "Yandex Cloud Folder ID where resources will be created"
}

variable "subnets_addresses" {
  type        = map(string)
  description = "Yandex Cloud load balancer target group contains subnets to addresses map "
}

variable "tg_name_prefix" {
  type        = string
  description = "Name prefix of target group"
}

