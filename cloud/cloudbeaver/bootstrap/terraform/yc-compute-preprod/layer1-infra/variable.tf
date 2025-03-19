variable "cloud_id" {
  type    = string
  default = "aoe4gl1v3n10v4vpst7d"
}

variable "folder_id" {
  type    = string
  default = "aoefk7qv9tpe23mejnlp"
}

variable "network_id" {
  type    = string
  default = "c6453k00df51004snnkd"
}

variable "s3_admin_access_key" {
  type        = string
  description = "s3-admin access-key"
}

variable "bootstrap_user_id" {
  type    = string
  default = "bfbghdcascvfmtqon9nq"
}

variable "s3_admin_secret_key" {
  type        = string
  description = "s3-admin secret-key"
}

variable "service_account_key_file" {
  type    = string
  default = "./sa.json"
}

locals {
  ycp_config = pathexpand("./ycp_config.yaml")
}

locals {
  v4_healthchecks_cidrs = [
    "198.18.235.0/24",
    "198.18.248.0/24",
  ]
  v6_healthchecks_cidrs = {
    prod    = ["2a0d:d6c0:2:ba::/80"],
    preprod = ["2a0d:d6c0:2:ba:ffff::/80"],
    testing = ["2a0d:d6c0:2:ba:1::/80"],
    gpn     = ["2a0d:d6c0:2:ba:200:0:c612:eb00/122 "],
  }
}
