module "salt-personal" {
  source = "../modules/services/salt_personal/v1"

  installation = local.installation
  salts = {
    "d0uble" = {
      override_shortname_prefix                   = "mdb-deploy-salt-d0uble"
      override_instance_env_suffix_with_emptiness = true
      override_zone_shortname_with_letter         = true
      zones                                       = ["zone_a"]
      override_resources                          = null
      boot_disk_spec                              = null
      override_boot_disks = {
        "01.zone_a" = data.yandex_compute_disk.mdb-deploy-salt-d0uble01k_boot
      }
      disable_seccomp = true
    }
    "trootnev" = {
      override_shortname_prefix                   = "mdb-deploy-salt-trootnev"
      override_instance_env_suffix_with_emptiness = true
      override_zone_shortname_with_letter         = true
      zones                                       = ["zone_a"]
      override_resources                          = null
      boot_disk_spec                              = null
      override_boot_disks = {
        "01.zone_a" = data.yandex_compute_disk.mdb-deploy-salt-trootnev01k_boot
      }
      disable_seccomp = true
    }
    "arhipov" = {
      override_shortname_prefix                   = "mdb-deploy-salt-arhipov"
      override_instance_env_suffix_with_emptiness = true
      override_zone_shortname_with_letter         = true
      zones                                       = ["zone_a"]
      override_resources = {
        core_fraction = 100
        cores         = 2
        memory        = 4
      }
      boot_disk_spec = {
        size     = 30
        image_id = "fdvu8jafbacv8b6rs4vl"
        type_id  = "network-ssd"
      }
      override_boot_disks = {
        // TODO: https://paste.yandex-team.ru/3987470 Wanna fix it? Be my guest! :)
        "FAKE_UNUSED_KEY" = data.yandex_compute_disk.mdb-deploy-salt-d0uble01k_boot
      }
      disable_seccomp = false
    }
    "velom" = {
      zones = ["zone_a"]
      boot_disk_spec = {
        size     = 30
        image_id = "fdv10th24ce4ia4kg679"
        type_id  = "network-ssd"
      }
      override_boot_disks = {
        // TODO: https://paste.yandex-team.ru/3987470 Wanna fix it? Be my guest! :)
        "FAKE_UNUSED_KEY" = data.yandex_compute_disk.mdb-deploy-salt-d0uble01k_boot
      }
      override_shortname_prefix                   = null
      override_instance_env_suffix_with_emptiness = false
      override_zone_shortname_with_letter         = false
      override_resources                          = null
      disable_seccomp                             = false
    }
  }
}

data "yandex_compute_disk" "mdb-deploy-salt-d0uble01k_boot" {
  disk_id = "a7liq4dudl4i65ktjidp"
}
data "yandex_compute_disk" "mdb-deploy-salt-trootnev01k_boot" {
  disk_id = "a7lh2tnpmekfjr0lhva5"
}
