variable "application_version" {
  description = "Version of docker image with kms-control-plane/kms-data-plane"
  default     = "25754-d58356a58b"
}

variable "tool_version" {
  description = "Version of docker image with kms-tool, use application_version unless there is a good reason not to"
  default     = "feature-CLOUD-96292-kms-global--26836-43543e2434"
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

variable "underlay_image_id" {
  default = "fdvn3v37ojk4bnfgnkgb"
}

variable "overlay_image_id" {
  default = "fdv1qvj4tac91b004mn8"
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

variable "jaeger-agent_image_version" {
  default = "1.19.2"
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

variable "host_group" {
  default = "service"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "hostname_suffix" {
  default = "kms.cloud-preprod.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
}

variable "ycp_profile" {
  default = "preprod"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1-c"
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
    env     = "pre-prod"
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
