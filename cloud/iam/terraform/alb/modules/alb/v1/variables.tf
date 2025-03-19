variable "name" {
  type = string
}

variable "folder_id" {
  type = string
}

variable "listener_name" {
  type    = string
  default = "tls"
}

variable "endpoint_ports" {
  type    = list(number)
  default = [443]
}

variable "external_ipv6_addresses" {
  type = list(string)
}

variable "external_ipv4_addresses" {
  type    = list(string)
  default = []
}

variable "certificate_ids" {
  type = list(string)
}

variable "https_router_id" {
  type = string
}

variable "http_router_id" {
  type    = string
  default = null
}

variable "is_edge" {
  type = bool
}

variable "sni_handlers" {
  type = list(object({
    name            = string
    server_names    = list(string)
    certificate_ids = list(string)
    http_router_id  = string
    is_edge         = bool
  }))
}

variable "allocation" {
  type = object({
    region_id  = string
    network_id = string
    locations  = list(object({
      subnet_id = string
      zone_id   = string
    }))
  })
}

variable "solomon_cluster_name" {
  type = string
}

variable "juggler_host" {
  type = string
}
