variable "application_version" {
  description = "Version of docker image with kms-control-plane/kms-data-plane"
  default     = "22033-5fee14a04e"
}

variable "tool_version" {
  description = "Version of docker image with kms-tool, use application_version unless there is a good reason not to"
  default     = "22033-5fee14a04e"
}

##########

variable "underlay_image_id" {
  default = "fd8mvfpf90bbncfjcbb0"
}

variable "overlay_image_id" {
  default = "fd8qg2anb3gbl46d8sbg"
}

##########

variable "metadata_image_version" {
  default = "82cc2cb932"
}

##########

variable "config_server_image_version" {
  default = "8cafa809ba"
}

variable "api_gateway_image_version" {
  default = "8cafa809ba"
}

variable "envoy_image_version" {
  default = "v1.14.7-8-g78f90a7"
}

##########

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "hostname_suffix" {
  default = "kms.cloud.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
}

variable "ycp_profile" {
  default = "prod"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1-c"
}

variable "yc_region" {
  default = "ru-central1"
}

variable "abc_group" {
  default = "yckms"
}

variable "instance_labels" {
  default = {
    layer   = "paas"
    abc_svc = "yckms"
    env     = "prod"
  }
}

variable "osquery_tag" {
  default = "ycloud-svc-kms"
}

##########

variable "yc_zone_suffix" {
  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}
