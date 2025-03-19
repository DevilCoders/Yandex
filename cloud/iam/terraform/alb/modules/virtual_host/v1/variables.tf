variable "name" {
  type = string
}

variable "authority" {
  type = list(string)
}

variable "https_router_id" {
  type = string
}

variable "https_ports" {
  type    = list(number)
  default = null
}

variable "http_router_id" {
  type    = string
  default = null
}

variable "modify_request_headers" {
  type    = object({
    name    = string
    remove  = bool
    replace = string
  })
  default = null
}

variable "routes" {
  type = list(object({
    name             = string
    is_http          = bool
    prefix_match     = string
    backend_group_id = string
  }))
}
