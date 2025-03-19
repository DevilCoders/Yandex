module "defaults" {
  source = "../modules/constants/defaults"
}

locals {
  iam_ya_dev_folder = {
    id = "b1g3a13lm14bku4da0pd" # yc-iam-internal-dev
  }

  iam_services = {
    "as"       = {
      authority = [
        "as.dev.cloud-internal.yandex.net",
        "as-new.dev.cloud-internal.yandex.net",
      ]
    }
    "iam"      = {
      authority = [
        "iam.dev.cloud-internal.yandex.net",
        "iam-new.dev.cloud-internal.yandex.net",
      ]
    }
    "identity" = {
      virtual_host_name_suffix = "identity-private"
      authority                = [
        "identity.dev.cloud-internal.yandex.net",
        "identity-new.dev.cloud-internal.yandex.net",
      ]
    }
    "oauth"    = {
      virtual_host_name_suffix = "auth"
      authority                = [
        "auth.dev.cloud-internal.yandex.net",
      ]
    }
    "org"      = {
      authority = [
        "org.dev.cloud-internal.yandex.net",
      ]
    }
    "reaper"   = {
      authority = [
        "reaper.dev.cloud-internal.yandex.net",
        "reaper-new.dev.cloud-internal.yandex.net",
      ]
    }
    "rm"       = {
      authority = [
        "rm.dev.cloud-internal.yandex.net",
        "rm-new.dev.cloud-internal.yandex.net",
      ]
    }
    "ss"       = {
      authority = [
        "ss.dev.cloud-internal.yandex.net",
      ]
    }
    "ti"       = {
      authority = [
        "ti.dev.cloud-internal.yandex.net",
        "ti-new.dev.cloud-internal.yandex.net",
      ]
    }
    "ts"       = {
      authority = [
        "ts.dev.cloud-internal.yandex.net",
        "ts-new.dev.cloud-internal.yandex.net",
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
