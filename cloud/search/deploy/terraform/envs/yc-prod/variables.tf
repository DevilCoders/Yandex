variable "cloud_id" {
  description = "ID of a cloud"
  default     = "yc.search"
  type        = string
}

variable "folder_id" {
  description = "ID of a folder"
  default     = "b1gnek8fpi8pqaf42pg6"
  type        = string
}

variable "main_zone" {
  description = "The main availability zone"
  default     = "ru-central1-a"
  type        = string
}

variable "default_labels" {
  description = "Set of labels"
  default     = { "env" = "prod", "deployment" = "terraform" }
  type        = map(string)
}

variable "ycp_profile" {
  default = "selfhost-profile"
}

variable "ycp_config" {
  default = ""
}

locals {
  ycp_config = (var.ycp_config != "" ? var.ycp_config : pathexpand("../configs/compute_prod.yaml"))
}

variable "service_account_key_file" {
  type    = string
  default = "./sa.json"
}

variable "s3_admin_access_key" {
  type        = string
  description = "s3-admin access-key"
}

variable "s3_admin_secret_key" {
  type        = string
  description = "s3-admin secret-key"
}

variable "selfdns_api_host" {
  default = "selfdns-api.yandex.net"
}

variable "service_account_id" {
  default = "ajeh1ndqle39oqbfl4du"
}

variable "region_id" {
  type    = string
  default = "ru-central1"
}

variable "instance_platform" {
  type    = string
  default = "standard-v2"
}

variable "instance_name_suffix" {
  default = "-prod0{instance.index_in_zone}-{{instance.zone_id}_shortname}"
}

variable "domain" {
  default = ".yandexcloud.net"
}

variable "selfdns_secret" {
  default = "e6qgb1hnluvipiut7dcm"
}

variable "sa_consumer_secret" {
  default = "e6q0ve0j2r71db1lt73l"
}

variable "sa_consumer_version" {
  default = "e6qnl7p5mdfa0co1r32b"
}

variable "lockbox_api_host" {
  default = "payload.lockbox.api.cloud.yandex.net"
}
