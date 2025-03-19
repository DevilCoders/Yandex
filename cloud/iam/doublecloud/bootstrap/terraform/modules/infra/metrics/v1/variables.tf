variable "vpc_id" {
  type = string
}

variable "amp_subnet_ids" {
  type = list(string)
}

variable "amp_security_group_ids" {
  type = list(string)
}

variable "k8s_cluster_name" {
  type = string
}

variable "openid_cluster_url" {
  type = string
}

variable "openid_cluster_arn" {
  type = string
}

variable "prometheus_reader_accounts" {
  type = list(string)
}
