variable "cloud_id" {
  type = string
}

variable "folder_id" {
  type = string
}

variable "zone" {
  default = "ru-central1-a"
}

variable "token" {
  type = string
}

variable "subnet_v4_cidr_block" {
  default = "10.0.0.0/16"
}

variable "nat" {
  default = true
}

variable "image_family" {
  default = "windows-2019-dc-gvlk"
}

variable "cores" {
  default = 2
}

variable "memory" {
  default = 4
}

variable "disk_size" {
  default = 50
}

variable "disk_type" {
  default = "network-ssd"
}

variable "admin_pass" {
  type = string
}

variable "timeout_create" {
  default = "10m"
}

variable "timeout_delete" {
  default = "10m"
}
