module "defaults" {
  source = "../modules/constants/defaults"
}

locals {
  iam_ya_prod_folder = {
    id = "b1g3lrojb296i4ppcen0" # yc-iam-internal-prod
  }

  iam_services = {
    "as"       = {
      authority = [
        "as.cloud.yandex-team.ru",
        "as-new.cloud.yandex-team.ru",
      ]
    }
    "iam"      = {
      authority = [
        "iamcp.cloud.yandex-team.ru",
        "iamcp-new.cloud.yandex-team.ru",
      ]
    }
    "identity" = {
      backend_port             = 443
      virtual_host_name_suffix = "identity-private"
      authority                = [
        "iam.cloud.yandex-team.ru",
        "iam-new.cloud.yandex-team.ru",
      ]
    }
    "oauth"    = {
      virtual_host_name_suffix = "auth"
      authority                = [
        "auth.cloud.yandex-team.ru",
      ]
    }
    "org"      = {
      authority = [
        "org.cloud.yandex-team.ru",
      ]
    }
    "reaper"   = {
      authority = [
        "reaper.cloud.yandex-team.ru",
        "reaper-new.cloud.yandex-team.ru",
      ]
    }
    "rm"       = {
      authority = [
        "rm.cloud.yandex-team.ru",
        "rmcp-new.cloud.yandex-team.ru",
      ]
    }
    "ss"       = {
      authority = [
        "ss.cloud.yandex-team.ru",
        "ss-new.cloud.yandex-team.ru",
      ]
    }
    "ti"       = {
      authority = [
        "ti.cloud.yandex-team.ru",
        "ti-new.cloud.yandex-team.ru",
      ]
    }
    "ti-idm"   = {
      authority = [
        "idm.ti.cloud.yandex-team.ru",
        "idm-new.ti.cloud.yandex-team.ru",
      ]
    }
    "ts"       = {
      authority = [
        "ts.cloud.yandex-team.ru",
        "ts-new.cloud.yandex-team.ru",
      ]
    }
  }

  iam_service_settings = {
    for k, v in local.iam_services : k => merge(
      lookup(module.defaults.iam_backends_defaults, k, {}),
      v
    )
  }
}
