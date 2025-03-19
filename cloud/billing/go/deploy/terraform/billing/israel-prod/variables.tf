variable "cgroup" {
  type        = string
  default     = "cloud_prod_billing-il"
  description = "Billing public production israel conductor group"
}

variable "checks_host" {
  type = string
  default = "yc_billing_il-prod"
}

variable "enabled_phone_notifications" {
  type    = bool
  default = true
}
