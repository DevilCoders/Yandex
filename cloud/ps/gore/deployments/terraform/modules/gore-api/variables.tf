variable "folder_id" {
  type        = string
  description = "The ID of folder"
  default     = "yc.gore.service-folder"
}

variable "network_id" {
  type        = string
  description = "The ID of network"
}

variable "environment" {
  type        = string
  description = "Gore environment: prod, preprod, debug"
}

variable "docker_image_version" {
  type        = string
  description = "Version of Docker image cr.yandex/crp1m3ip7sn09t6912j9/gore"
}

variable "dns_zone_id" {
  type        = string
  description = "The ID of DNS zone"
}

variable "service_account_id" {
  type        = string
  description = "The ID of service account for VMs"
}

variable "subnet_ids" {
  type        = map(string)
  description = "Map of subnet IDs"
}

variable "tvm_secret" {
  type = string
}

variable "mongo_users" {
  type = map(string)
  sensitive = true
  default = {
    "gore"    = "",
    "andgein" = "",
  }
}

variable "api_domain" {
  type        = string
  description = "Domain for GoResps API, i.e. resps-api.cloud.yandex.net. Certificate will be issued for this domain, as well as ALB will be installed"
}

variable "need_dns_record" {
  type = bool
  description = "Create A record for GoResps API domain"
}
