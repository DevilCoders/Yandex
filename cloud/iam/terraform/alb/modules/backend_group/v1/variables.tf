variable "is_http" {
  type    = bool
  default = false
}

variable "name" {
  type = string
}

variable "backend_name" {
  type    = string
  #  If not specified, "var.name" will be used.
  default = null
}

variable "description" {
  type    = string
  default = null
}

variable "backend_port" {
  type = number
}

variable "healthcheck_port" {
  type = number
}

variable "http_healthcheck_path" {
  type = string
}

variable "healthcheck_healthy_threshold" {
  type    = number
  default = 2
}

variable "healthcheck_unhealthy_threshold" {
  type    = number
  default = 3
}

variable "healthcheck_interval" {
  type    = string
  default = "1s"
}

variable "healthcheck_timeout" {
  type    = string
  default = "0.500s"
}

variable "backend_weight" {
  type = number
}

variable "folder_id" {
  type = string
}

variable "target_group_id" {
  type = string
}

variable "environment" {
  type = string
}
