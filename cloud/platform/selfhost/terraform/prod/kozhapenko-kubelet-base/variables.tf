// SSH keys Specific
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

// Service Specific

variable "image_id" {
  default = "fd8aojotg0jduet9i005"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "4"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "16"
}

variable "zones" {
  type = "list"

  default = [
    "ru-central1-c",
  ]
}

variable "host_suffix_for_zone" {
  type = "map"

  default = {
    "ru-central1-c" = "myt"
  }
}

variable "subnets" {
  type = "map"

  default = {
    "ru-central1-c" = "b0c7crr1buiddqjmuhn7"
  }
}

// Provider Specific
variable "yc_token" {
  description = "Yandex Cloud security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  default     = "b1gnamrk6its2tl2ntap"                                   //kozhapenko
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisoned resources"
  default     = "ru-central1-c"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud.yandex.net:443"
}

variable "docker_auth" {
  description = "docker_auth_string for registry.yandex.net (base64 encoded login:token)"
}

variable "cert_file" {
  description = <<EOF
Local path to envoy certificate file. Contents for this file can be
donwloaded from https://yav.yandex-team.ru/secret/
EOF


  default = "_intentionally_empty_file"
}

variable "key_file" {
  description = <<EOF
Local path to envoy certificate private key file. Contents for this file can be
donwloaded from https://yav.yandex-team.ru/secret/
EOF


  default = "_intentionally_empty_file"
}
