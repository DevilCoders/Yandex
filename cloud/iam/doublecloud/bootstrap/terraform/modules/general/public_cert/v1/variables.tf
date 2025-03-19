variable "domain_name" {
  type        = string
  description = "Domain name for certificate CN"
}

variable "public_zone_id_for_acme_challenge" {
  type        = string
  description = "Route53 zone ID to perform ACME DNS-01 challenge"
}

variable "subject_alternative_names" {
  type = list(string)
  default = []
  description = "SAN list. Validation DNS records should be filled by hands"
}
