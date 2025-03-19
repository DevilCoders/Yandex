variable "name" {
  type        = string
  description = "Name of target group"
}

variable "ycp_profile" {
  type = string
}

variable "folder" {
  type        = string
  description = "Yandex Cloud Folder ID where resources will be created"
}

variable "zone" {
  type        = string
  description = "Yandex Cloud default Zone for provisioned resources"
}

variable "subnets_addresses" {
  type        = map(string)
  description = "Yandex Cloud load balancer target group contains subnets to addresses map "
}
