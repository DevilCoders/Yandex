variable "swiss_knife_version" {
  description = "Version of docker image with cr.yandex/crp2dqh2mcnp87a3uce6/schecker-swiss-knife"
  default     = "20220719102924-cc2ee75c05"
}

variable "syncer_version" {
  description = "Version of docker image with cr.yandex/crp2dqh2mcnp87a3uce6/schecker-syncer"
  default     = "20220719102924-cc2ee75c05"
}

variable "parser_version" {
  description = "Version of docker image with cr.yandex/crp2dqh2mcnp87a3uce6/schecker-parser"
  default     = "20220719102924-cc2ee75c05"
}

variable "api_version" {
  description = "Version of docker image with cr.yandex/crp2dqh2mcnp87a3uce6/schecker-api"
  default     = "20220719102924-cc2ee75c05"
}

##########

variable "push-client_image_version" {
  default = "2021-02-03T14-09"
}

##########

variable "overlay_image_id" {
  default = "fd8hb2v10o84n3n53rlt"
}

##########

variable "host_group" {
  default = "service"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = ["ru-central1-c"]
}

variable "hostname_suffix" {
  default = "schecker.cloud.yandex.net"
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

variable "abc_group" {
  default = "yc-schecker"
}

variable "instance_labels" {
  default = {
    layer   = "paas"
    abc_svc = "yc-schecker"
    env     = "prod"
  }
}

variable "osquery_tag" {
  default = "ycloud-svc-schecker"
}

##########

variable "yc_zone_suffix" {
  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}
