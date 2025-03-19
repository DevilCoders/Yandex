variable "name" {
  type = string
}

variable "vpc_id" {
  type = string
}

variable "protocol" {
  type = string
}

variable "protocol_version" {
  type    = string
  default = null
}

variable "port" {
  type = number
}

variable "target_type" {
  type = string
}

variable "lb_arn" {
  type = string
}

variable "certificate_arn" {
  type    = string
  default = null
}

variable "alpn_policy" {
  type    = string
  default = null
}

variable "health_check" {
  type = object({
    protocol = string
    port     = string
    path     = string
    matcher  = string
  })
}
