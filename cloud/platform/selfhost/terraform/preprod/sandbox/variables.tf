variable "public_key_path" {
  description = "Path to public key file"
}

variable "yc_token" {
  description = "Yandex Cloud security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  default     = "aoepl0tck9rnsitci6hi"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisoned resources"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = " api.cloud-preprod.yandex.net:443"
}
