variable "token" {
  type = string
}

variable "cloud_id" {
  type = string
}

variable "folder_id" {
  type = string
}

variable "zone" {
  default = "ru-central1-a"
}

variable "subnet_v4_cidr_block" {
  default = "10.0.0.0/16"
}

variable "nat" {
  default = true
}

variable "image_family" {
  default = "ubuntu-20-04-lts-rdpx"
}

variable "cores" {
  default = 4
}

variable "memory" {
  default = 8
}

variable "disk_size" {
  default = 50
}

variable "disk_type" {
  default = "network-ssd"
}

variable "username" {
  type = string
}

variable "password" {
  type = string
}

variable "ssh_key" {
  type        = string
  default     = "~/.ssh/id_rsa.pub"
  description = "path to ssh public key"
}
