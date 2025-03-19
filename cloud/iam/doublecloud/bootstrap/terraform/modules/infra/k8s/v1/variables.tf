variable "aws_account" {
  type = string
}

variable "aws_profile" {
  type = string
}

variable "cluster_name" {
  type = string
}

variable "ssh_key_name" {
  type = string
}

variable "cluster_subnet_ids" {
  type = list(string)
}

variable "node_groups" {
  type = list(object({
    name               = string
    subnet_ids         = list(string)
    security_group_ids = list(string)
    labels             = map(string)

    instance_types     = list(string)
    scaling_config     = object({
      desired_size = number
      max_size     = number
      min_size     = number
    })
  }))
}

variable "ami_version" {
  type = string

  default = null
}

variable "cluster_encryption_config" {
  type = list(object({
    provider_key_arn = string
    resources        = list(string)
  }))
  default = []
}

variable "ingress_cidrs" {
  type = list(string)
}

variable "ingress_ipv6_cidrs" {
  type = list(string)
}

variable "iam_devops" {
  type = list(string)
}

variable "auth_ui_devops" {
  type = list(string)
}
