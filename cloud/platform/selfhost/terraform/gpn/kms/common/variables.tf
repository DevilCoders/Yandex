variable "application_version" {
  description = "Version of docker image with kms-control-plane/kms-data-plane"
  default     = "22283-a6999f70f7"
}

variable "tool_version" {
  description = "Version of docker image with kms-tool, use application_version unless there is a good reason not to"
  default     = "22283-a6999f70f7"
}

variable "monitoring_version" {
  description = "Version of docker image with kms-monitoring"
  default     = "22283-a6999f70f7"
}

##########

variable "overlay_image_id" {
  default = "d8o1m4bgr3vdibllbqlk"
}

##########

variable "metadata_image_version" {
  default = "82cc2cb932"
}

variable "solomon_agent_image_version" {
  default = "2021-02-03T14-11"
}

variable "push-client_image_version" {
  default = "2021-02-03T14-09"
}

##########

variable "config_server_image_version" {
  default = "8cafa809ba"
}

variable "api_gateway_image_version" {
  default = "8cafa809ba"
}

variable "envoy_image_version" {
  default = "v1.12.3-19-g260063f"
}

##########

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = ["ru-gpn-spb99"]
}

variable "hostname_suffix" {
  default = "kms.gpn.yandexcloud.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
}

variable "ycp_profile" {
  default = "gpn-fed"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-gpn-spb99"
}

variable "yc_region" {
  default = "ru-gpn-spb"
}

variable "abc_group" {
  default = "yckms"
}

variable "instance_labels" {
  default = {
    layer   = "paas"
    abc_svc = "yckms"
    env     = "private-gpn"
  }
}

variable "osquery_tag" {
  default = "ycloud-svc-kms"
}

##########

variable "yc_zone_suffix" {
  default = {
    "ru-gpn-spb99" = "gpn"
  }
}
