module "defaults" {
  source = "../modules/constants/defaults"
}

locals {
  environment          = "israel"
  region_id            = "il1"
  stand_domain         = "cloudil.co.il"
  auth_fqdn            = "auth.${local.stand_domain}"
  auth_dns_record_name = "${local.auth_fqdn}."

  openid_folder = {
    id = "yc.iam.openid-server-folder"
  }

  auth_ui_backend_group = {
    id = "a7p128qf2df0q7tpcltm"
  }

  vpc = {
    auth = {
      network = {
        id = "ccmvfqn1frp1hq8dfiug"
      }
      subnets = {
        "a" : {
          id      = "ddksj8ga56p7c97po92j"
          zone_id = "il1-a"
        }
      }
    }
  }

  dns = {
    zone_id = "yc.iam.auth"
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
