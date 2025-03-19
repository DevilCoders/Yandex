##########

variable "image_id" {
  type = string
}

variable "cluster" {
  type = string
}

########## Compute config

variable "yc_folder" {
  type = string
}

variable "service_account_id" {
  type = string
}

variable "security_group_ids" {
  type = list(string)
}

variable "instances_amount" {
  type = string
}

variable "ipv6_addresses" {
  type = list(string)
}

variable "ipv4_addresses" {
  type = list(string)
}

variable "subnets" {
  type = map(string)
}

variable "platform_id" {
  type = string
}

variable "instance_cores" {
  type = string
  description = "Cores per one instance"
}

variable "instance_core_fraction" {
  description = "Core fraction per one instance"
  default     = 100
}

variable "instance_memory" {
  type = string
  description = "Memory in GB per one instance"
}

variable "instance_disk_size" {
  type = string
  description = "Boot disk size in GB per one instance"
}

variable "instance_disk_type" {
  type = string
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
}

variable "labels" {
  type = object({
    env     = string
    layer   = string
    abc_svc = string
  })
}

########## Application config

variable "database" {
  type = object({
    host = string
    port = string
    name = string
    user = string
  })
}

variable "smtp" {
  type = object({
    addr = string
    user = string
  })
}

variable "kinesis" {
  type = object({
    endpoint    = string
    region      = string
    stream_name = string
    key_id      = string
  })
}

########## Other

variable "host_group" {
  type = string
}

variable "yc_zones" {
  type = list(string)
  description = "Yandex Cloud Zones to deploy in"
}

variable "dns_zone" {
  type = string
}

variable "instance_platform_id" {
  type = string
}

variable "ycp_profile" {
  type = string
}

variable "ycp_zone" {
  type = string
}

variable "abc_group" {
  type = string
}

variable "solomon_storage_limit" {
  type = string
}

##########

variable "yc_zone_suffixes" {
  type = map(string)
}

##########

# Must be set via command line params.
variable "yandex_token" {
  type = string
  description = "Yandex Team security OAuth token"
}
