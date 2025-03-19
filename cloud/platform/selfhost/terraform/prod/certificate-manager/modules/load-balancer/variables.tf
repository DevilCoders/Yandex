variable "yc_folder" {
  type        = string
  description = "Yandex Cloud Folder ID where resources will be created"
}

variable "lb_name_prefix" {
  type        = string
  description = "Name prefix of load balancer and target group"
}

variable "ip_version" {
  type        = string
  description = "IP version ipv4 or ipv6"
}

variable "ip_address" {
  type        = string
  description = "IP address"
}

variable "port" {
  type        = string
  description = "port"
}

variable "yandex_only" {
  type        = string
  description = "yandex_only"
}

variable "target_group_id" {
  type        = string
  description = "target group id"
}

variable "health_check_path" {
  description = "Path used by LB for healthcheck"
  default     = "/ping"
}

variable "health_check_port" {
  description = "Port used by LB for healthcheck"
  default     = "9982"
}
