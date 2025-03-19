variable "application_version" {
  description = "Version of docker image with lockbox-control-plane"
  default     = "25472-865a874766"
}

variable "image_id" {
  default = "fdvpspahq62dl1laj63q"
}

////////

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-lockbox/default
  default = "aoe2fnrknrpgsho3chc5"
}

variable "service_account_id" {
  // yc-lockbox/lockbox
  default = "bfbc91a56dngtkj4epph"
}

variable "ycp_profile" {
  default = "preprod"
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

  // VLA, MYT
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:501:0:f810:0:*
  // SAS: 2a02:6b8:c02:901:0:f806:0:*
  // MYT: 2a02:6b8:c03:501:0:f810:0:*
  default = ["2a02:6b8:c0e:501:0:fc26:0:101", "2a02:6b8:c02:901:0:fc26:0:101", "2a02:6b8:c03:501:0:fc26:0:101"]
}

variable "subnets_ipv6_addresses" {
  type = map(string)

  default = {
    "buc91ifa8c8cdec85vr7" = "2a02:6b8:c0e:501:0:fc26:0:101"
    "blt3gb55j2e4egmstlf9" = "2a02:6b8:c02:901:0:fc26:0:101"
    "fo2jqre0qrrjgk1ssbuf" = "2a02:6b8:c03:501:0:fc26:0:101"
  }
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  default = ["172.16.0.101", "172.17.0.101", "172.18.0.101"]
}

variable "subnets_ipv4_addresses" {
  type = map(string)

  default = {
    "buc91ifa8c8cdec85vr7" = "172.16.0.101"
    "blt3gb55j2e4egmstlf9" = "172.17.0.101"
    "fo2jqre0qrrjgk1ssbuf" = "172.18.0.101"
  }
}

variable "yc_region" {
  default  = "ru-central1"
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "buc91ifa8c8cdec85vr7"
    "ru-central1-b" = "blt3gb55j2e4egmstlf9"
    "ru-central1-c" = "fo2jqre0qrrjgk1ssbuf"
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
  default = ["c641egpce17hsqrosvr3"]
}

variable "ycp_prod" {
  description = "True if this is a PROD installation"
  default     = "false"
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
  default = "lockbox.cloud-preprod.yandex.net"
}

variable "lb_public_api_port" {
  description = "Balancer public API port"
  default     = "443"
}

variable "lb_private_api_port" {
  description = "Balancer private API port"
  default     = "8443"
}

variable "private_api_port" {
  description = "Must be the same as grpcServer port in application.yaml"
  default     = "9443"
}

variable "health_check_path" {
  description = "Path used by LB for healthcheck"
  default     = "/"
}

variable "private_health_check_port" {
  description = "Port used by LB for healthcheck"
  default     = "8444"
}

variable "health_check_port" {
  description = "Port used by LB for healthcheck"
  default     = "444"
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
  default     = "8"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "20"
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
  default = "2021-02-03T14-11"
}

variable "push-client_image_version" {
  default = "2021-02-03T14-09"
}

variable "config_server_image_version" {
  type    = string
  default = "0ed0dee310"
}

variable "api_gateway_image_version" {
  type    = string
  default = "0ed0dee310"
}

variable "envoy_image_version" {
  type    = string
  default = "v1.12.3-19-g260063f"
}
