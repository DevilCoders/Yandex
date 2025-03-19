variable "aws_profile" {
  type = string
}

variable "infra" {
  type = any
}

variable "env_name" {
  type = string
}

variable "cloud_provider" {
  type = string
}

variable "k8s_cluster_name" {
  type = string
}

variable "iam_default_security_group_id" {
  type = string
}

variable "region_suffix" {
  type = string
}

variable "fqdn_operations" {
  type = string
}

variable "fqdn_internal_iam" {
  type = string
}

variable "fqdn_internal_iam_alternatives" {
  type    = list(string)
  default = []
}

variable "zone_internal_iam" {
  type = string
}

variable "fqdn_public_auth" {
  type = string
}

variable "zone_public_auth" {
  type = string
}

variable "zone_private_iam" {
  type = string
}

variable "endpoint_service_acceptance_required" {
  type = bool
}

variable "endpoint_service_allowed_principals" {
  type = list(string)
}

variable "fqdn_notify" {
  type = string
}

variable "zone_notify" {
  type = string
}

variable "notify_endpoint_service_name" {
  type = string
}

variable "billing_endpoint_fqdn" {
  type = string
}

variable "billing_endpoint_service_name" {
  type = string
}

variable "zone_mdb" {
  type = string
}

variable "mdb_endpoint_fqdn" {
  type = string
}

variable "mdb_endpoint_service_name" {
  type = string
}

variable "zone_vpc" {
  type = string
}

variable "vpc_endpoint_fqdn" {
  type = string
}

variable "vpc_endpoint_service_name" {
  type = string
}
