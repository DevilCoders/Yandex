variable "application_version" {
  description = "Version of docker image with lockbox-control-plane/lockbox-data-plane"
  default     = "25472-865a874766"
}

variable "ydb-dumper_version" {
  description = "Version of docker image with ydb-dumper"
  default     = "25472-865a874766"
}

##########

variable "overlay_image_id" {
  default = "alkuluhjnjj3dotif9kd"
}

##########

variable "metadata_image_version" {
  default = "82cc2cb932"
}

variable "solomon_agent_image_version" {
  default = "2022-06-10T20-10"
}

##########

variable "config_server_image_version" {
  default = "127c8a0162"
}

variable "api_gateway_image_version" {
  default = "127c8a0162"
}

variable "envoy_image_version" {
  default = "v1.14.4-12-g1844ad1"
}

##########

variable "local_lb_fqdn" {
  default = "local-lb.yandexcloud.co.il"
}

variable "local_lb_addr" {
  # ip address of il1-a-ct3-26b.infra.yandexcloud.co.il.
  default = "2a11:f740::103:bace:f6ff:fe44:86ea"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = ["il1-a"]
}

variable "hostname_suffix" {
  default = "lockbox.crypto.yandexcloud.co.il"
}

variable "instance_platform_id" {
  default = "standard-v3"
}

variable "ycp_profile" {
  default = "israel"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "il1-a"
}

variable "yc_region" {
  default = "il1"
}

variable "abc_group" {
  default = {
    abc_service = "yckms",
    abc_service_scopes = ["administration"]
  }
}

variable "instance_labels" {
  default = {
    layer   = "paas"
    abc_svc = "yckms"
    env     = "israel"
  }
}

variable "osquery_tag" {
  default = "ycloud-svc-kms"
}

##########

variable "yc_zone_suffix" {
  default = {
    "il1-a" = "il1a"
  }
}
