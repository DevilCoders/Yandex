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
  default     = "aoe56gngo7aoaa45nrga" // managed-kubernetes
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisoned resources"
  default     = "ru-central1-c"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud-preprod.yandex.net:443"
}

// Instance secrets

variable "docker_auth" {
  description = <<EOF
docker_auth_string for registry.yandex.net (base64 encoded login:token)
You can get it from https://yav.yandex-team.ru/secret/sec-01cx8a81rj3458rhqaj5x1sztb
EOF

  default = ""
}

variable "juggler_token" {
  description = <<EOF
OAuth token for interaction with juggler to manage downtimes.
You can get it from https://yav.yandex-team.ru/secret/sec-01dqfyqhpf6vbsbtkfk02nswkc
EOF

  default = ""
}

variable "solomon_token" {
  description = <<EOF
OAuth token for interaction with solomon to push user cluster metrics.
You can get it from  https://yav.yandex-team.ru/secret/sec-01dwf7ypx34h8z2sf89kqhbqyj 
EOF

  default = ""
}

