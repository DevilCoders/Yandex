variable "application_version" {
  description = "Version of docker image with kms-control-plane/kms-data-plane"
  default     = "24042-fe83918e49"
}

variable "tool_version" {
  description = "Version of docker image with kms-tool, use application_version unless there is a good reason not to"
  default     = "24042-fe83918e49"
}

variable "monitoring_version" {
  description = "Version of docker image with kms-monitoring"
  default     = "24042-fe83918e49"
}

variable "root_kms_version" {
  description = "Version of docker image with kms-root-service"
  default     = "20258-3847c615da"
}

##########

variable "overlay_image_id" {
  default = "c2pdkuo5ro6p1d6pkrb9"
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
  # VLA, MYT
  default = ["ru-central1-a", "ru-central1-c"]
}

variable "hostname_suffix" {
  default = "kms.cloud-testing.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
}

variable "ycp_profile" {
  default = "testing"
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
    env     = "testing"
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
