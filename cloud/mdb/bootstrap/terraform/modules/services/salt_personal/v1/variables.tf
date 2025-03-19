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

variable "salts" {
  type = map(object({
    override_shortname_prefix                   = string
    override_instance_env_suffix_with_emptiness = bool
    override_zone_shortname_with_letter         = bool
    zones                                       = list(string)
    override_resources = object({
      core_fraction = number
      cores         = number
      memory        = number
    })
    boot_disk_spec = object({
      size     = number
      image_id = string
      type_id  = string
    })
    override_boot_disks = map(any)
    disable_seccomp     = bool
  }))
}

variable "platform_id" {
  type = string

  default = null
}
