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
  default     = "b1gu1l0o1atnq0b2uo37" // yandexcloud / cloudvm
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud.yandex.net:443"
}

// Instance params

variable "image_id" {
  // that should be docker + service to start teamcity like docker container
  default = "fd8g49t06fgtle259oeo"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "8"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "32"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "600"
}

variable "instance_description" {
  description = "Textual explanation of the instance role"
  default     = "Teamcity agent (https://nda.ya.ru/3VnuTW)"
}

variable "skip_update_ssh_keys" {
  description = "If true, instances will be labeled and sandbox job won't update ssh keys them"
  default     = "false"
}

// Instance group placement params
variable "instance_count" {
  description = "number of instances"
  default     = "20"
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
    "ru-central1-a" = "e9bgoij6333u3ha229f8"
    "ru-central1-b" = "e2lc39edk785ntro14ja"
    "ru-central1-c" = "b0ca1se1qbg4mbrofo04"
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

variable "ipv6_addrs" {
  type = list(string)

  default = [
    "2a02:6b8:c0e:500:0:f805:0:ea01",
    "2a02:6b8:c02:900:0:f805:0:ea02",
    "2a02:6b8:c03:500:0:f805:0:ea03",
    "2a02:6b8:c0e:500:0:f805:0:ea04",
    "2a02:6b8:c02:900:0:f805:0:ea05",
    "2a02:6b8:c03:500:0:f805:0:ea06",
    "2a02:6b8:c0e:500:0:f805:0:ea07",
    "2a02:6b8:c02:900:0:f805:0:ea08",
    "2a02:6b8:c03:500:0:f805:0:ea09",
    "2a02:6b8:c0e:500:0:f805:0:ea10",
    "2a02:6b8:c02:900:0:f805:0:ea11",
    "2a02:6b8:c03:500:0:f805:0:ea12",
    "2a02:6b8:c0e:500:0:f805:0:ea13",
    "2a02:6b8:c02:900:0:f805:0:ea14",
    "2a02:6b8:c03:500:0:f805:0:ea15",
    "2a02:6b8:c0e:500:0:f805:0:ea16",
    "2a02:6b8:c02:900:0:f805:0:ea17",
    "2a02:6b8:c03:500:0:f805:0:ea18",
    "2a02:6b8:c0e:500:0:f805:0:ea19",
    "2a02:6b8:c02:900:0:f805:0:ea20",
  ]
}

// Instance secrets

variable "docker_auth" {
  description = <<EOF
docker_auth_string for registry.yandex.net (base64 encoded login:token)
see secrets.tf for more details
EOF
  default = ""
}

variable "docker_auth_cr_yandex" {
  description = <<EOF
docker_auth_string for cr.yandex (base64 encoded type-of-login:token)
see secrets.tf for more details
EOF
  default = ""
}

