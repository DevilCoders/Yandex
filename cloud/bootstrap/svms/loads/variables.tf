variable "vms" {
  type = map(number)
  default = {
    target = 1
    tank   = 1
  }
}

variable "ssh_agent" {
  type    = bool
  default = false
}

variable "ssh_private_key" {
  type    = string
  default = ""
}

variable "bastion_host" {
  type    = string
  default = null
}

//TODO: switch to cloud-dns&oslogin or use standard stable image for service-images
variable "boot_disk_id" {
  type    = string
  default = "fdv67uacau0plmc44vvr"
}

variable "selfdns_token" {
  type = string
}
