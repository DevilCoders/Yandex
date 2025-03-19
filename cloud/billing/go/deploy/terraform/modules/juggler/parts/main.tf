variable "system" {
  type = string
}

variable "env" {
  type = string
}

variable "notify_delay" {
  type    = string
  default = "5m"
}

output "flaps" {
  value = {
    short = {
      critical_time = 300
      stable_time   = 60
    }
    long = {
      critical_time = 1200
      stable_time   = 240
    }
  }
}

locals {
  default_tags = [
    "yc-billing-terraform",
    "yc-billing",
    "yc",
    "yc-${var.env}",
    "yc-billing-${var.system}"
  ]
}

output "tags" {
  value = {
    default = local.default_tags
    notify  = concat(local.default_tags, ["yc-notify", "yc-notify-delay-${var.notify_delay}"])
  }
}
