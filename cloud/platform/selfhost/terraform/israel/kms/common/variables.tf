variable "application_version" {
  description = "Version of docker image with kms-control-plane/kms-data-plane"
  default     = "25472-865a874766"
}

variable "tool_version" {
  description = "Version of docker image with kms-tool, use application_version unless there is a good reason not to"
  default     = "25472-865a874766"
}

variable "monitoring_version" {
  description = "Version of docker image with kms-monitoring"
  default     = "25472-865a874766"
}

variable "ydb-dumper_version" {
  description = "Version of docker image with ydb-dumper"
  default     = "25472-865a874766"
}

##########

variable "overlay_image_id" {
  default = "alkqr0f2dnsghfhg0l6q"
}

##########

variable "metadata_image_version" {
  default = "82cc2cb932"
}

variable "solomon_agent_image_version" {
  default = "2022-06-10T20-10"
}

##########

variable "local_lb_fqdn" {
  default = "local-lb.yandexcloud.co.il"
}

variable "local_lb_addr" {
  # ip address of vla04-3ct5-11a.cloud.yandex.net
  default = "2a02:6b8:bf00:340:bace:f6ff:fe44:8126"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = ["il1-a"]
}

variable "hostname_suffix" {
  default = "kms.crypto.yandexcloud.co.il"
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
