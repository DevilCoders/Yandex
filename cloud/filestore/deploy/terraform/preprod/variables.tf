variable "token" {
  type        = string
  description = "Yandex Cloud token. Use iam token for it (create from yc)"
}

variable "image_version" {
  type        = string
  description = "image version to deploy"
  default     = "stable-21-2"
}
