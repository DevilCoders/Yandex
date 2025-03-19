module "defaults" {
  source = "../../modules/constants/defaults"
}

locals {
  environment  = "hw-blue-lab"
  region_id    = "ru-central1"
  stand_domain = "hw-blue.cloud-lab.yandex.net"
  auth_fqdn    = "auth.${local.stand_domain}"

  openid_folder = {
    id = "yc.iam.openid-server-folder"
  }

  vpc = {
    auth = {
      network = {
        id        = "ehjv37i2k76enhaimptu"
      }
      subnets = {
        "a" : {
          id      = "fru1c8nm8gg3tojak9j5"
          zone_id = "ru-central1-a"
        }
        "b" : {
          id      = "fru1hnf9jln4gp6nvlp8"
          zone_id = "ru-central1-b"
        }
        "c" : {
          id      = "fruqqaqjkaqe6es0n751"
          zone_id = "ru-central1-c"
        }
      }
    }
  }

  iam_services = {
    "oauth" = {
      virtual_host_name_suffix = "auth"
      authority                = [
        local.auth_fqdn,
      ]
      target_group_id          = ycp_platform_alb_target_group.iam_openid_target_group.id
    }
  }

  iam_service_settings = {
    for k, v in local.iam_services : k => merge(
      lookup(module.defaults.iam_backends_defaults, k, {}),
      v
    )
  }
}
