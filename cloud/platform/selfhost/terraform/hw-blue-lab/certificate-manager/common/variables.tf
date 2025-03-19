variable "application_version" {
  description = "Version of docker image with certificate-manager-control-plane/certificate-manager-data-plane"
  default     = "24239-c5dc23a9ff"
}

variable "tool_version" {
  description = "Version of docker image with certificate-manager-tool, use application_version unless there is a good reason not to"
  default     = "24239-c5dc23a9ff"
}

variable "ydb-dumper_version" {
  description = "Version of docker image with ydb-dumper"
  default     = "20865-d718499aa1"
}

##########

variable "image_id" {
  default = "dcp18ren5hv6efgs6hio"
}

##########

variable "metadata_image_version" {
  default = "82cc2cb932"
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
  default = "cert-manager.hw-blue.cloud-lab.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
}

variable "ycp_profile" {
  default = "hw-blue-lab-ycm"
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
    env     = "hw-blue-lab"
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
