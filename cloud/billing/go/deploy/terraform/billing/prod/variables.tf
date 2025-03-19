variable "cgroup" {
  type        = string
  default     = "cloud_prod_billing"
  description = "Billing prod conductor group"
}

variable "checks_host" {
  type = string
  default = "yc_billing_prod"
}

variable "enabled_phone_notifications" {
  type    = bool
  default = true
}
