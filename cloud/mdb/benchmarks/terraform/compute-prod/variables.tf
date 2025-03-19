variable "cloud_id" {
  type    = string
  default = "b1gvm30ove44ga99ona1"
}

variable "folder_id" {
  type    = string
  default = "b1g6p6mdrv3ol7qm3qml"
}

variable "region_id" {
  type    = string
  default = "ru-central1"
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

