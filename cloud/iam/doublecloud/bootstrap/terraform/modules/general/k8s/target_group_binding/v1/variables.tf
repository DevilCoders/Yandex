variable "namespace" {
  type = string
}

variable "name" {
  type    = string
  default = null
}

variable "service" {
  type = string
}

variable "port" {
  type = number
}

variable "target_group_arn" {
  type = string
}
