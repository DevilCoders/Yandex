variable "aws_account" {
  type = string
}

variable "aws_profile" {
  type = string
}

variable "infra_name" {
  type = string
}

variable "region" {
  type = string
}

variable "k8s_config" {
  type = object({
    cluster_name = string
    ssh_key_name = string
    node_groups  = list(object({
      name               = string
      instance_types     = list(string)
      scaling_config     = object({
        desired_size = number
        max_size     = number
        min_size     = number
      })
    }))
  })
}

variable "private_zones" {
  type = list(string)
}

variable "public_zones" {
  type = list(string)
}

variable "delegation_set_reference_name" {
  type = string
}

variable "zone_tech_iam" {
  type = string
}

variable "iam_devops" {
  type = list(string)
}

variable "auth_ui_devops" {
  type = list(string)
}

variable "skip_dns_public_resources" {
  type    = bool
  default = false
}

variable "skip_metrics_resources" {
  type    = bool
  default = false
}

variable "transit_gateways" {
  type    = list(object({
    transit_gateway_id = string
    cidr_block         = string
  }))
}

variable "prometheus_reader_accounts" {
  type = list(string)
}
