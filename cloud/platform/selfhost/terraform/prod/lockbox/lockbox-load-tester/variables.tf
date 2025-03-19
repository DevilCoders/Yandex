variable "application_version" {
  description = "Version of docker image with lockbox-data-plane"
  default     = "xxx"
}

variable "image_id" {
  default = "fd8qg2anb3gbl46d8sbg"
}

////////

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud.yandex.net:443"
}

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_token" {
  description = "Yandex Cloud security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-lockbox/default
  default = "b1gk7vfm46orverufgc5"
}

variable "service_account_id" {
  // yc-lockbox/lockbox
  default = "ajeelub63qujkkag2lia"
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
  // VLA: 2a02:6b8:c0e:500:0:f845:d3bb:0/112
  // SAS: 2a02:6b8:c02:900:0:f845:d3bb:0/112
  // MYT: 2a02:6b8:c03:500:0:f845:d3bb:0/112
  default = ["2a02:6b8:c0e:500:0:f845:d3bb:205", "2a02:6b8:c02:900:0:f845:d3bb:205", "2a02:6b8:c03:500:0:f845:d3bb:205"]
}

variable "subnets_ipv6_addresses" {
  type = map(string)

  default = {
    "e9ba19ffma25gqffbmos" = "2a02:6b8:c0e:500:0:f845:d3bb:205"
    "e2ldetr4j3jq08ahuddo" = "2a02:6b8:c02:900:0:f845:d3bb:205"
    "b0c07vthi8i9fcm4lfmu" = "2a02:6b8:c03:500:0:f845:d3bb:205"
  }
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.0/16
  // SAS: 172.17.0.0/16
  // MYT: 172.18.0.0/16
  default = ["172.16.0.205", "172.17.0.205", "172.18.0.205"]
}

variable "subnets_ipv4_addresses" {
  type = map(string)

  default = {
    "e9ba19ffma25gqffbmos" = "172.16.0.205"
    "e2ldetr4j3jq08ahuddo" = "172.17.0.205"
    "b0c07vthi8i9fcm4lfmu" = "172.18.0.205"
  }
}

variable "yc_region" {
  default  = "ru-central1"
}

variable "subnets" {
  type = map

  default = {
    "ru-central1-a" = "e9ba19ffma25gqffbmos"
    "ru-central1-b" = "e2ldetr4j3jq08ahuddo"
    "ru-central1-c" = "b0c07vthi8i9fcm4lfmu"
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
  default = ["enpdn39rq3f4g8rum7pd"]
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
  default = "lockbox.cloud.yandex.net"
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

variable "health_check_port" {
  description = "Port used by LB for healthcheck"
  default     = "8444"
}

variable "instance_platform_id" {
  default = "standard-v1"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "16"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "16"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "10"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

variable "metadata_image_version" {
  type    = string
  default = "1b931b4a1"
}

variable "solomon_agent_image_version" {
  type    = string
  default = "2020-01-22T09-25"
}

variable "push-client_image_version" {
  type    = string
  default = "2020-10-02T13-49"
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
