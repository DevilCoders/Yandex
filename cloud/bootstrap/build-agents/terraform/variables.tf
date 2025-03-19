variable "build_agents_per_dc" {
  type = map(number)
  default = {
    vla = 15
    myt = 15
    sas = 10
  }
}

variable "ssh_agent" {
  type    = bool
  default = false
}

variable "ssh_user" {
  type = string
}

variable "ssh_private_key" {
  type    = string
  default = ""
}

variable "bastion_host" {
  type    = string
  default = null
}

variable "snapshot_id" {
  type    = string
  default = "fdv3ude063knkh50i5o4"
}

variable "boot_disk_id" {
  type    = string
  default = "fdvb2kea78c3mcc8vr3p"
}
