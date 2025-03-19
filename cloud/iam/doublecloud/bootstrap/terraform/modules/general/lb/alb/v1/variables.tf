variable "name" {
  type = string
}

variable "vpc_id" {
  type = string
}

variable "subnets_ids" {
  type = list(string)
}

variable "security_groups_ids" {
  type = list(string)
}

variable "internal" {
  type = bool
}

variable "ip_address_type" {
  type = string

  validation {
    condition     = var.ip_address_type == "ipv4" || var.ip_address_type == "dualstack"
    error_message = "The 'ip_address_type' must be one of 'ipv4' or 'dualstack'."
  }
}

variable "targets" {
  type = map(object({
    service_name     = string
    port             = number
    port_name        = string
    protocol         = string
    protocol_version = string
    health_check     = object({
      protocol = string
      path     = string
      port     = number
      matcher  = string
    })
  }))
}

variable "target_type" {
  type = string

  validation {
    condition     = var.target_type == "ip" || var.target_type == "instance"
    error_message = "The 'target_type' must be one of 'ip' or 'instance'."
  }
}

variable "service_type" {
  type = string
}

variable "certificate_arn" {
  type = string
}
