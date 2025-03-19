variable "cgroup" {
  type        = string
  default     = "cloud_prod_billing-ya"
  description = "Billing internal prestable conductor group"
}

variable "checks_host" {
  type = string
  default = "yc_billing_internal-prod"
}

variable "enabled_phone_notifications" {
  type    = bool
  default = true
}
