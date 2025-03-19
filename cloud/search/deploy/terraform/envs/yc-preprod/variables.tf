variable "cloud_id" {
  description = "ID of a cloud"
  default     = "yc.search"
  type        = string
}

variable "folder_id" {
  description = "ID of a folder"
  default     = "aoemlggmretqinbok8ee"
  type        = string
}

variable "main_zone" {
  description = "The main availability zone"
  default     = "ru-central1-a"
  type        = string
}

variable "default_labels" {
  description = "Set of labels"
  default     = { "env" = "preprod", "deployment" = "terraform" }
  type        = map(string)
}

variable "ycp_profile" {
  default = "selfhost-profile"
}

variable "ycp_config" {
  default = ""
}

locals {
  ycp_config = (var.ycp_config != "" ? var.ycp_config : pathexpand("../configs/compute_preprod.yaml"))
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
  default = "bfbp4lr5j1sakrsrdaut"
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
  default = "-preprod0{instance.index_in_zone}-{{instance.zone_id}_shortname}"
}

variable "domain" {
  default = ".cloud-preprod.yandex.net"
}

variable "selfdns_secret" {
  default = "fc3d7oiobdrt73l90t5h"
}

variable "sa_consumer_secret" {
  default = "fc3r6isjnekouqgrbc7o"
}

variable "sa_consumer_version" {
  default = "fc3rbf7jeuv0mpo81dpr"
}

variable "lockbox_api_host" {
  default = "payload.lockbox.api.cloud-preprod.yandex.net"
}
