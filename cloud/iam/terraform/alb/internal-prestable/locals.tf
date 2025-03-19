module "defaults" {
  source = "../modules/constants/defaults"
}

locals {
  iam_ya_prestable_folder = {
    id = "b1gptblrmcgr1k2dbda3" # yc-iam-internal-prestable
  }

  iam_services = {
    "as"       = {
      authority = [
        "as.prestable.cloud-internal.yandex.net",
        "as-new.prestable.cloud-internal.yandex.net",
      ]
    }
    "iam"      = {
      authority = [
        "iam.prestable.cloud-internal.yandex.net",
        "iam-new.prestable.cloud-internal.yandex.net",
      ]
    }
    "identity" = {
      virtual_host_name_suffix = "identity-private"
      authority                = [
        "identity.prestable.cloud-internal.yandex.net",
        "identity-new.prestable.cloud-internal.yandex.net",
      ]
    }
    "oauth"    = {
      virtual_host_name_suffix = "auth"
      authority                = [
        "auth.prestable.cloud-internal.yandex.net",
      ]
    }
    "org"      = {
      authority = [
        "org.prestable.cloud-internal.yandex.net",
      ]
    }
    "reaper"   = {
      authority = [
        "reaper.prestable.cloud-internal.yandex.net",
        "reaper-new.prestable.cloud-internal.yandex.net",
      ]
    }
    "rm"       = {
      authority = [
        "rm.prestable.cloud-internal.yandex.net",
        "rm-new.prestable.cloud-internal.yandex.net",
      ]
    }
    "ss"       = {
      authority = [
        "ss.prestable.cloud-internal.yandex.net",
      ]
    }
    "ti"       = {
      authority = [
        "ti.prestable.cloud-internal.yandex.net",
        "ti-new.prestable.cloud-internal.yandex.net",
      ]
    }
    "ts"       = {
      authority = [
        "ts.prestable.cloud-internal.yandex.net",
        "ts-new.prestable.cloud-internal.yandex.net",
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
