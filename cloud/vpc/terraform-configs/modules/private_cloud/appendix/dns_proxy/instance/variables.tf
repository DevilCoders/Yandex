variable "folder_id" {
  type = string
  description = "Ihe id of the folder for new instances"
}

variable "image_id" {
  type = string
  description = "The id of the image for instances"
}

variable "placement_group_id" {
  type = string
  description = "The id of the placement group for instances"
  default = ""
}

variable "vm_count" {
  type = number
  description = "Amount of instances to create"
  default = 1
}

variable "dns_zone" {
  type = string
  description = "Dns zone for instances, i.e. svc.cloud-testing.yandex.net. Note: instances DONT register themselves via selfdns api" 
}

variable "prefix" {
  type = string
  description = "Prefix for names of created entities. I.e. gpn"
}

variable "suffix" {
  type = string
  description = "suffix for names of created entities. I.e. vla"
  default = ""
}

variable "zone_id" {
  type = string
  description = "Zone for creating instances. I.e. ru-central1-a"
}

variable "interconnect_subnet_id" {
  type = string
  description = "The id of the subnet for interconnecting with private cloud"
}

variable "dmzprivcloud_subnet_id" {
  type = string
  description = "The id of the subnet for connecting with Yandex"
}

variable "interconnect_addresses" {
  type = list(string)
  description = "IPv6 addresses in interconnect subnet"
}

variable "dmzprivcloud_addresses" {
  type = list(string)
  description = "IPv6 addresses in dmzprivcloud subnet"
}
