variable "service_name" {
  description = "Name of the service. Must be unique."
  type        = string
}

variable "installation" {
  type = object({
    instance_env_suffix = string
    dns_zone            = string
    region_id           = string
    known_zones = map(object({
      shortname = string
      letter    = string
      subnet_id = string
      id        = string
    }))
    security_groups = list(string)
  })
}

variable "zones" {
  type = list(string)

  default = []
}

variable "allow_stopping_for_update" {
  type = bool

  default = true
}

variable "platform_id" {
  type = string

  default = "standard-v2"
}

variable "resources" {
  type = object({
    core_fraction = number
    cores         = number
    memory        = number
  })
}

variable "instances_per_zone" {
  type = number

  default = 1
}

variable "boot_disk_spec" {
  type = object({
    size     = number
    image_id = string
    type_id  = string
  })
}

variable "override_shortname_prefix" {
  type = string

  default = null
}

variable "override_instance_env_suffix_with_emptiness" {
  type = bool

  default = false
}

variable "override_zone_shortname_with_letter" {
  type = bool

  default = false
}

variable "override_boot_disks" {
  type = any

  default = {}
}

variable "override_load_balancer_ipv6_addr" {
  type = string

  default = null
}

variable "create_load_balancer" {
  type = bool

  default = false
}

variable "override_security_groups" {
  type = list(string)

  default = []
}

variable "disable_seccomp" {
  type = string

  default = false
}

variable "pci_topology_id" {
  type = string

  default = null
}
