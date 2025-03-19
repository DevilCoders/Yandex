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
  default     = "b1ggokjh96von6jdu14e" // translateapi-gateway
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisoned resources"
  default     = "ru-central1-c"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud.yandex.net:443"
}

// Instance params

variable "image_id" {
  default = "fd8ildiqenls12immr26"
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
  default     = "80"
}

// Green instance params

variable "green_image_id" {
  default = "fd8ildiqenls12immr26"
}

variable "green_instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "green_instance_memory" {
  description = "Memory in GB per one instance"
  default     = "4"
}

variable "green_instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "80"
}

// Instance group placement params

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "e9b9e47n23i7a9a6iha7"
    "ru-central1-b" = "e2lt4ehf8hf49v67ubot"
    "ru-central1-c" = "b0c7crr1buiddqjmuhn7"
  }
}

// Instance secrets

variable "docker_auth" {
  description = <<EOF
docker_auth_string for registry.yandex.net (base64 encoded login:token)
You can get it from https://yav.yandex-team.ru/secret/sec-01cx8a81rj3458rhqaj5x1sztb
EOF


  default = ""
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

