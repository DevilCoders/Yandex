// SSH keys Specific

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

// Provider Specific

variable "yc_token" {
  description = "Yandex Cloud security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  default     = "aoee5o6qaidham4dtjuj" // jaegermeister
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisoned resources"
  default     = "ru-central1-c"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud-preprod.yandex.net:443"
}

// Instance params

variable "image_id" {
  default = "fdv7ov9khk4991rs6783"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "1"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "2"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "40"
}

variable "instance_description" {
  description = "Textual explanation of the instance role"
  default     = "jaegermeister server"
}

variable "skip_update_ssh_keys" {
  description = "If true, instances will be labeled and sandbox job won't update ssh keys them"
  default     = "true"
}

// Instance group placement params

variable "instance_count" {
  description = "number of instances"
  default     = "3"
}

variable "zones" {
  type = list(string)

  default = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ]
}

variable "zone_subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "bucpba0hulgrkgpd58qp"
    "ru-central1-b" = "bltueujt22oqg5fod2se"
    "ru-central1-c" = "fo27jfhs8sfn4u51ak2s"
  }
}

variable "zone_suffix" {
  type = map(string)

  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}

variable "zone_regions" {
  type = map(string)

  default = {
    "ru-central1-a" = "ru-central1"
    "ru-central1-b" = "ru-central1"
    "ru-central1-c" = "ru-central1"
  }
}

variable "ipv6_addrs" {
  type = list(string)

  default = [
    "2a02:6b8:c0e:501:0:f806:0:50",
    "2a02:6b8:c02:901:0:f806:0:50",
    "2a02:6b8:c03:501:0:f806:0:50",
  ]
}

variable "ipv4_addrs" {
  type = list(string)

  default = [
    "172.16.0.80",
    "172.17.0.80",
    "172.18.0.80",
  ]
}

// Instance secrets

variable "aws_access_key" {
  description = <<EOF
Access key ID to s3 bucket with route configurations
You can get it from https://yav.yandex-team.ru/secret/sec-01cxcwpq2hwygkxrqq6v73077q
EOF


  default = ""
}

variable "aws_secret_key" {
  description = <<EOF
Secret to s3 bucket with route configurations
You can get it from https://yav.yandex-team.ru/secret/sec-01cxcwpq2hwygkxrqq6v73077q
EOF


default = ""
}

variable "docker_auth" {
description = <<EOF
docker_auth_string for registry.yandex.net (base64 encoded login:token)
You can get it from https://yav.yandex-team.ru/secret/sec-01cx8a81rj3458rhqaj5x1sztb
EOF


default = ""
}

