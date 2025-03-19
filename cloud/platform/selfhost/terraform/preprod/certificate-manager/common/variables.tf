variable "application_version" {
  description = "Version of docker image with certificate-manager-control-plane/certificate-manager-data-plane"
  default     = "26325-467544aa3b"
}

variable "tool_version" {
  description = "Version of docker image with certificate-manager-tool, use application_version unless there is a good reason not to"
  default     = "26325-467544aa3b"
}

variable "ydb-dumper_version" {
  description = "Version of docker image with ydb-dumper"
  default     = "26325-467544aa3b"
}

##########

variable "image_id" {
  default = "fdvlppljlpr9sjpj7oc0"
}

##########

variable "metadata_image_version" {
  default = "82cc2cb932"
}

variable "solomon_agent_image_version" {
  default = "2021-07-07T15-31"
}

variable "push-client_image_version" {
  default = "2021-07-07T15-31"
}

variable "metricsagent_version" {
  default = "be43027525"
}

##########

variable "config_server_image_version" {
  default = "78d5cdfd34"
}

variable "api_gateway_image_version" {
  default = "78d5cdfd34"
}

variable "envoy_image_version" {
  default = "v1.14.7-8-g78f90a7"
}

##########

variable "host_group" {
  default = "service"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c"]
}

variable "hostname_suffix" {
  default = "cert-manager.cloud-preprod.yandex.net"
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

variable "yc_region" {
  default = "ru-central1"
}

variable "abc_group" {
  default = {
    abc_service = "yccertificatemanager",
    abc_service_scopes = ["administration"]
  }
}

variable "instance_labels" {
  default = {
    layer   = "paas"
    abc_svc = "yccertificatemanager"
    env     = "pre-prod"
  }
}

variable "osquery_tag" {
  default = "ycloud-svc-certificate-manager"
}

##########

variable "yc_zone_suffix" {
  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}
