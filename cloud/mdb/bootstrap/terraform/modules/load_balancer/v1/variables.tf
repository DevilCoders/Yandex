variable "load_balancer_name" {
  description = "Name of the load balancer. Must be unique."
  type        = string
}

variable "region_id" {
  type = string
}

variable "ipv6_addr" {
  type = string
}

variable "targets" {
  type = list(any)
}
