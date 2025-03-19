variable "application_version" {
  description = "Version of docker image with crypto-e2e-monitoring"
  default     = "25575-8b61fbad36"
}

variable "image_id" {
  default = "fd8loccl1l5jp63es9mp"
}

////////

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-crypto-e2e-monitoring/default
  default = "b1gnv4ho2qqit6j1ilnc"
}

variable "service_account_id" {
  // yc-crypto-e2e-monitoring/root
  default = "aje83g1rn9v1lv2qetqp"
}

variable "ycp_profile" {
  default = "prod"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1-a"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"

  // VLA, SAS, MYT
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:501:0:f810:0:*
  // SAS: 2a02:6b8:c02:901:0:f806:0:*
  // MYT: 2a02:6b8:c03:501:0:f810:0:*
  default = ["2a02:6b8:c0e:500:0:f862:0:13", "2a02:6b8:c02:900:0:f862:0:13", "2a02:6b8:c03:500:0:f862:0:13"]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  default = ["172.16.0.13", "172.17.0.13", "172.18.0.13"]
}

variable "yc_region" {
  default  = "ru-central1"
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "e9btepibuu8dt9h99ark"
    "ru-central1-b" = "e2lj8df713c2linii8kb"
    "ru-central1-c" = "b0cgmc3dcdecc3jns4r0"
  }
}

variable "yc_zone_suffix" {
  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}

variable "host_group" {
  default = "service"
}

variable "security_group_ids" {
  default = ["xxx"]
}

variable "ycp_prod" {
  description = "True if this is a PROD installation"
  default     = "true"
}

#variable "ycp_profile" {
#  description = "Name of ycp profile, useful only for testing. Please name your testing profile 'testing'"
#  default     = "testing"
#}

# variable "yc_endpoint" {
#   description = "Yandex Cloud API endpoint"
#   default     = "api.cloud-testing.yandex.net:443"
# }

variable "hostname_suffix" {
  default = "crypto.cloud.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "4"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "50"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

variable "metadata_image_version" {
  type    = string
  default = "82cc2cb932"
}

variable "solomon_agent_image_version" {
  type    = string
  default = "2020-11-24T12-55"
}

variable "push-client_image_version" {
  type    = string
  default = "2020-11-24T12-59"
}
