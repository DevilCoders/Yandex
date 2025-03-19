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
  default     = "b1gq57e4mu8e8nhimac0" // mqtt
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
  default = "fd8l2he8gbbm1ned50m7"
}

variable "events_instance_cores" {
  description = "Cores per one events instance"
  default     = "2"
}

variable "events_instance_memory" {
  description = "Memory in GB per one events broker instance"
  default     = "2"
}

// MQTT
variable "mqtt_instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "mqtt_instance_memory" {
  description = "Memory in GB per one instance"
  default     = "2"
}

variable "mqtt_instance_description" {
  description = "Textual explanation of the instance role"
  default     = "MQTT server"
}

// Devices
variable "devices_instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "devices_instance_memory" {
  description = "Memory in GB per one instance"
  default     = "2"
}

variable "devices_instance_description" {
  description = "Textual explanation of the instance role"
  default     = "Device management service"
}

// Common

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "10"
}

variable "data_disk_size" {
  description = "Data disk size in GB per one instance"
  default     = "6"
}

variable "skip_update_ssh_keys" {
  description = "If true, instances will be labeled and sandbox job won't update ssh keys them"
  default     = "true"
}

variable "push_client_version" {
  type    = string
  default = "2019-03-28T18-40"
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

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "e9b9e47n23i7a9a6iha7"
    "ru-central1-b" = "e2lt4ehf8hf49v67ubot"
    "ru-central1-c" = "b0c7crr1buiddqjmuhn7"
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

variable "devices_cert_file" {
description = <<EOF
Local path to devices application certificate file. Content of this file can be
donwloaded from https://yav.yandex-team.ru/secret/sec-01d61132ge8dnfagfwgnwx17hv
EOF


default = "_intentionally_empty_file"
}

variable "mqtt_cert_file" {
description = <<EOF
Local path to mqtt certificate file. Content of this file can be
donwloaded from https://yav.yandex-team.ru/secret/sec-01d5xt5m7pnygvesvrz0nbvgv7
EOF


  default = "_intentionally_empty_file"
}

variable "mqtt_key_file" {
  description = <<EOF
Local path to mqtt certificate private key file. Content of this file can be
donwloaded from https://yav.yandex-team.ru/secret/sec-01d5xt5m7pnygvesvrz0nbvgv7
EOF


  default = "_intentionally_empty_file"
}

variable "events_sa_id" {
  type = string
  default = "ajeo7dqrvtlnic46fv8p"
}

variable "mqtt_sa_id" {
  type = string
  default = "aje7s14jmivmruolknc7"
}

variable "config_dump_aws_access_key" {
  description = <<EOF
Access key ID to s3 bucket with config dumps
You can get it from https://yav.yandex-team.ru/secret/sec-01d0vzntewrtxm7x932wefwkrm
EOF


default = ""
}

variable "config_dump_aws_secret_key" {
description = <<EOF
Secret to s3 bucket with config dumps
You can get it from https://yav.yandex-team.ru/secret/sec-01d0vzntewrtxm7x932wefwkrm
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

