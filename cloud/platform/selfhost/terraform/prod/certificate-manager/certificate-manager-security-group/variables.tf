variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud.yandex.net:443"
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
  default     = "b1g35p5i7e2sm037rgu1"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1"
}

variable "network_id" {
  description = "Network id"
  default     = "enpqohi7oct0gtqbivg2"
}