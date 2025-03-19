variable "cgroup" {
  type        = string
  default     = "cloud_preprod_billing"
  description = "Billing preprod conductor group"
}

variable "checks_host" {
  type = string
  default = "yc_billing_preprod"
}

variable "enabled_phone_notifications" {
  type    = bool
  default = true
}
