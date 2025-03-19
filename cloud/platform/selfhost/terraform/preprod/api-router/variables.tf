// SSH keys Specific

variable "yandex_token" {
  description = "Yandex Team security OAuth token. Get one from https://sandbox.yandex-team.ru/oauth"
}

// Provider Specific

variable "yc_token" {
  description = "Yandex Cloud security OAuth token. Get one from https://nda.ya.ru/3URva3"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created. Default is 'api-router'"
  default     = "aoe55ujrkdnaeb9mpu33"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud-preprod.yandex.net:443"
}

// Instance params

variable "image_id" {
  default = "fdvgl3335k9km4lpvk6f"
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
  default     = "80"
}

variable "instance_description" {
  description = "Textual explanation of the instance role"
  default     = "envoy L7 balancer (will accept api.cloud-preprod.yandex.net)"
}

variable "skip_update_ssh_keys" {
  description = "If true, instances will be labeled and sandbox job won't update ssh keys them"
  default     = "false"
}

// Instance group placement params
variable "number_of_instances" {
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

variable "cert_file" {
  description = <<EOF
Local path to envoy certificate file. Contents for this file can be
donwloaded from https://yav.yandex-team.ru/secret/sec-01cxyt4z3ny73j61kg3xmj4fkd
EOF


  default = "_intentionally_empty_file"
}

variable "key_file" {
  description = <<EOF
Local path to envoy certificate private key file. Contents for this file can be
donwloaded from https://yav.yandex-team.ru/secret/sec-01cxyt4z3ny73j61kg3xmj4fkd
EOF


default = "_intentionally_empty_file"
}

variable "xds_client_cert_file" {
description = <<EOF
Local path to client certificate file to access XDS. Content of this file can be
donwloaded from https://yav.yandex-team.ru/secret/sec-01d16g9cr9kyqn8vvkcjsaa0ts
EOF


default = "_intentionally_empty_file"
}

variable "xds_client_key_file" {
description = <<EOF
Local path to client private key file to access XDS. Content of this file can be
donwloaded from https://yav.yandex-team.ru/secret/sec-01d16g9cr9kyqn8vvkcjsaa0ts
EOF


  default = "_intentionally_empty_file"
}

variable "docker_auth" {
  description = <<EOF
docker_auth_string for registry.yandex.net (base64 encoded login:token)
You can get it from https://yav.yandex-team.ru/secret/sec-01cx8a81rj3458rhqaj5x1sztb
EOF


  default = ""
}

