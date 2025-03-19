variable name {
  type = string
  # example = "cpl"
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
