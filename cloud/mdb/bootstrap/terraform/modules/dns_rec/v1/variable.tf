
variable "dns_zone_id" {
  type        = string
  description = "what has been given by cloud dns when creating DNS zone. See https://nda.ya.ru/t/EdBWJkBW4ALNFF for more."
}

variable "domain" {
  type        = string
  description = "domain name, before first dot"
}

variable "ingress_ips_v4" {
  type        = list(string)
  description = "IPv4 list of ingress"
  default     = []
}

variable "ingress_ips_v6" {
  type        = list(string)
  description = "IPv6 list of ingress"
  default     = []
}
