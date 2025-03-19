variable name {
  type = string
  # example = "cpl"
}

variable conductor_group_suffix {
  type = string
  # example = "l7-jaeger"
}

variable tracing_service {
  type = string
  # example: cpl-router
}

variable server_cert_pem {
  type = string
}

variable server_cert_key {
  type = string
}

variable alb_lb_id {
  type = string
}

variable ig_sa {
  type = string
}

variable instance_sa {
  type = string
}

variable fixed_size {
  default = 3
}

variable has_ipv4 {
  default = false
}

variable enable_tracing {
  default = true
}