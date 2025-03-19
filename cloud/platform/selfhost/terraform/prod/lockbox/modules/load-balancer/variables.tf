variable "name" {
  type        = string
  description = "Name of the balancer"
}

variable "ycp_profile" {
  type = string
}

variable "folder" {
  type        = string
  description = "Yandex Cloud Folder ID where resources will be created"
}

variable "zone" {
  type        = string
  description = "Yandex Cloud default Zone for provisioned resources"
}

variable "ip_address" {
  type        = string
  description = "IP address"
}

variable "ip_version" {
  type        = string
  description = "IP version ipv4 or ipv6"
}

variable "yandex_only" {
  type        = string
  description = "yandex_only"
}

variable "port" {
  type        = string
  description = "port"
}

variable "target_group_id" {
  type        = string
  description = "target group id"
}

variable "health_check_path" {
  type        = string
  description = "Health check path"
}

variable "health_check_port" {
  type        = string
  description = "Health check port"
}
