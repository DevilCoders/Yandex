variable "service_name" {
  type = string
}

variable "tag_name" {
  type = string
}

variable "vpc_id" {
  type = string
}

variable "subnet_ids" {
  type = list(string)
}

variable "security_group_ids" {
  type = list(string)
}

variable "route53_record_zone_id" {
  type = string
}

variable "route53_record_fqdn" {
  type = string
}
