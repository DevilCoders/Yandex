variable "folder_id" {
  type = string
  description = "Folder in yc-tools cloud to create monops Session SA, and IG"
}

variable "ycp_profile" {
  type = string
  description = "Name of the ycp profile"
}

variable "yc_endpoint" {
  type = string
  description = "Endpoint for skm"
}

variable "cr_endpoint" {
  type = string
  description = "Endpoint for container registry"
}

variable "hc_network_ipv6" {
  type = string
  description = "IPv6 healthcheck network"
}

variable "environment" {
  type = string
  description = "Name of cloud environment, i.e. prod or testing"
}

variable "oauth_endpoint" {
  type = string
  description = "Endpoint for OAuth Service"
}

variable "ss_endpoint" {
  type = string
  description = "Endpoint for Session Service"
}

variable "iam_endpoint" {
  type = string
  description = "Endpoint for IAM"
}

variable "as_host" {
  type = string
  description = "Endpoint host for Access Service"
}

variable "yav_viewer_sa_secrets" {
  type = string
  description = "Name of the yav secrets of viewer SAs"
  default = "sec-01epvxk5mqscnvksg4656yz8f2"
}

variable "yav_mongo_password" {
  type = string
  description = "Name of the yav secrets with mongo database passwords"
  default = "sec-01evw6krqsrf464xsfm7qstnwc"
}

variable "monops_host" {
  type = string
  description = "Host in monops web URL"
}

variable "auth_environments_bypass" {
  type = list(string)
  description = "List of environments which should use bypass auth"
}

variable "auth_environments_proxy" {
  type = list(string)
  description = "List of environments which should use proxy auth"
}

variable "monops_image_file" {
  type = string
  description = "Image file in yc-vpc-packer-export S3 bucket"
  default = "fd8mtv8iciebcijq5aoh.qcow2"
}

variable "monops_zones" {
  type = list(string)
  description = "List of zones where monops can be deployed"
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "monops_network_id" {
  type = string
  description = "List of network where monops web instances are plugged"
}

variable "monops_network_subnet_ids" {
  type = map(string)
  description = "List of subnets where monops web instances are plugged"
}

variable "monops_ipv6_address" {
  type = string
  description = "IPv6 address of monops load balancer"
}

variable "monops_healthcheck_port" {
  type = number
  description = "HTTP port for monops healthchecks"
  default = 8440
}

variable "monops_web_port" {
  type = number
  description = "HTTPS port for monops web"
  default = 8080
}

variable "dns_zone" {
  type = string
  description = "Domain part of generated instance hostnames (without leading dot)"
}

variable "dns_zone_id" {
  type = string
  description = "DNS Zone for Monops instances"
}
