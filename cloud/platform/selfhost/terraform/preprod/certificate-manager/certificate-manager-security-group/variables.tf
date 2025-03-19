variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud-preprod.yandex.net:443"
}

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_service_account_key_file" {
  description = "Yandex Cloud security service account key file"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  // yc-certificate-manager/default
  default     = "aoehdr81kgmuudc48h60"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1"
}

variable "network_id" {
  description = "Network id"
  default     = "c64o1lg39vtujj1br1ro"
}
