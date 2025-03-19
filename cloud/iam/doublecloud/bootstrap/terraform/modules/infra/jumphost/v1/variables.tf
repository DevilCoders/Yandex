variable "subnet_id" {
  type = string
}

variable "vpc_security_group_ids" {
  type = list(string)
}

variable "jumphost_name" {
  type = string
}

variable "ssh_key_name" {
  type = string
}

variable "instance_type" {
  type    = string
  default = "t2.nano"
}
