variable "cluster_name" {
  type = string
}

variable "ami_version" {
  type = string
}

variable "node_group_name" {
  type = string
}

variable "node_group_role" {
  type = any
}

variable "subnet_ids" {
  type = list(string)
}

variable "security_group_ids" {
  type = list(string)
}

variable "labels" {
  type = map(string)
}

variable "scaling_config" {
  type = object({
    desired_size = number
    max_size     = number
    min_size     = number
  })
}

variable "instance_types" {
  type = list(string)
}

variable "ssh_key_name" {
  type = string
}
