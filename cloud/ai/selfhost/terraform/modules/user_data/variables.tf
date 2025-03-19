variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "extra_bootcmd" {
  description = "List of extra commands to execute durind boot"
  type        = list(list(string))
  default     = []
}

variable "extra_runcmd" {
  description = "List of extra commands to execute durind first start"
  type        = list(list(string))
  default     = []
}

variable "extra_user_data" {
  description = "Additional user data"
  default     = {}
}

variable "abc_service" {
  description = "ABC service to populate users from"
  default     = "yc_ml_services"
}
