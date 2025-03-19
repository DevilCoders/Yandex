variable "application_version" {
  description = "Version of docker image with kms-control-plane/kms-data-plane"
  default     = "24239-c5dc23a9ff"
}

variable "tool_version" {
  description = "Version of docker image with kms-tool, use application_version unless there is a good reason not to"
  default     = "24239-c5dc23a9ff"
}

variable "monitoring_version" {
  description = "Version of docker image with kms-monitoring"
  default     = "23975-4b0feefc78"
}

variable "ydb-dumper_version" {
  description = "Version of docker image with ydb-dumper"
  default     = "23664-ceb71e6371"
}

##########

variable "overlay_image_id" {
  default = "dcpo8bj5ndfsulmbuade"
}

##########

variable "metadata_image_version" {
  default = "82cc2cb932"
}

##########

variable "local_lb_fqdn" {
  default = "local-lb.cloud-lab.yandex.net"
}

variable "local_lb_addr" {
  # ip address of vla04-3ct5-11a.cloud.yandex.net
  default = "2a02:6b8:bf00:340:bace:f6ff:fe44:8126"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "hostname_suffix" {
  default = "kms.hw-blue.cloud-lab.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
}

variable "ycp_profile" {
  default = "hw-blue-lab-kms"
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
    abc_service = "yckms",
    abc_service_scopes = ["administration"]
  }
}

variable "instance_labels" {
  default = {
    layer   = "paas"
    abc_svc = "yckms"
    env     = "hw-blue-lab"
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
