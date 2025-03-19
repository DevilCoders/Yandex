variable "image" {
  type        = "string"
  description = "Image name with tag. You can find last version here - https://teamcity.yandex-team.ru/repository/download/cloud_java_InstanceGroupPipeline_YaTeamApiAdapterBuildImage/.lastSuccessful/version.txt"
}

variable "yandex_api_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud-preprod.yandex.net:443"
}

variable "yandex_cloud_id" {
  default = "aoeolbmrnipjt7p7kf6p"
}

variable "yandex_folder_id" {
  default = "aoei2f6ptsqdaeu3ck7p"
}

variable "base_image_folder_id" {
  default = "aoe5k83dn6vak86d5a3i"
}

variable "base_image_id" {
  default = "fdv5bl7niab6iiljcbet"
}

variable "subnets" {
  type = "map"

  default = {
    "ru-central1-a" = "bucpba0hulgrkgpd58qp"
    "ru-central1-b" = "bltueujt22oqg5fod2se"
    "ru-central1-c" = "fo27jfhs8sfn4u51ak2s"
  }
}

variable "zone_suffix" {
  type = "map"

  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}
