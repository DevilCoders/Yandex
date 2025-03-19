variable "folder_id" {
  type = string
  description = "The ID of folder"
  default = "yc.gore.service-folder"
}

variable "environment" {
  type = string
  description = "Bot environment: prod, preprod, debug"
}

variable "docker_image_version" {
  type = string
  description = "Version of Docker image cr.yandex/crpkne7bucbk4155b9uf/dutybot"
}

variable "bot_id" {
  type = string
  description = "First part of Bot Telegram token"
}

variable "dns_zone_id" {
  type = string
  description = "The ID of DNS zone"
}

variable "service_account_id" {
  type = string
  description = "The ID of service account for VMs"
}

variable "network_id" {
  type = string
  description = "The ID of network"
}

variable "subnet_ids" {
  type = map(string)
  description = "Map of subnet IDs"
}

variable "run_yandex_messenger_bot" {
  type = bool
  description = "Set to false if you don't want to run Yandex Messenger Dutybot in this environment"
  default = true
}

variable "hc_network_ipv6" {
  type = string
  description = "IPv6 healthcheck network"
}
