variable "yandex_token" {
  type      = string
  sensitive = true
}

variable "hosts" {
  type        = list(string)
  description = "juggler checks hosts group"
}

variable "env" {
  type = string
}

variable "hosts_type" {
  type    = string
  default = "CGROUP"
}

variable "checks_host" {
  type = string
}

variable "enabled_phone_notifications" {
  type = bool
}
