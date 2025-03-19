variable "yandex_token" {
  type      = string
  sensitive = true
}

variable "hosts" {
  type        = list(string)
  description = "juggler checks hosts group"
}

variable "installation" {
  type = string
}

variable "enable_notifications" {
  type = bool
}
