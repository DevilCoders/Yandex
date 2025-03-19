module "salt-personal" {
  source = "../../../instance/v1"

  for_each = var.salts

  installation                                = var.installation
  service_name                                = "salt-personal-${each.key}"
  override_shortname_prefix                   = each.value.override_shortname_prefix
  override_instance_env_suffix_with_emptiness = each.value.override_instance_env_suffix_with_emptiness
  override_zone_shortname_with_letter         = each.value.override_zone_shortname_with_letter
  zones                                       = each.value.zones

  resources = {
    core_fraction = try(each.value.override_resources.core_fraction, 20)
    cores         = try(each.value.override_resources.cores, 2)
    memory        = try(each.value.override_resources.memory, 4)
  }

  boot_disk_spec      = each.value.boot_disk_spec
  override_boot_disks = each.value.override_boot_disks
  disable_seccomp     = each.value.disable_seccomp
}

terraform {
  required_version = ">= 0.14.7, < 0.15.0"
}
