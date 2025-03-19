variable "yc_folder" {
  type        = string
  description = "Yandex Cloud Folder ID where resources will be created"
}

variable "subnets_addresses" {
  type        = map(string)
  description = "Yandex Cloud load balancer target group contains subnets to addresses map "
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

variable "yandex_only" {
  type        = string
  description = "yandex_only"
}

variable "lb_public_api_port" {
  description = "Balancer public API port"
  default     = "443"
}

variable "lb_private_api_port" {
  description = "Balancer private API port"
  default     = "8443"
}

variable "health_check_path" {
  description = "Path used by LB for healthcheck"
  default     = "/ping"
}

variable "health_check_port" {
  description = "Port used by LB for healthcheck"
  default     = "9982"
}
