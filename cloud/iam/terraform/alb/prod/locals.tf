module "defaults" {
  source = "../modules/constants/defaults"
}

locals {
  openid_folder = {
    id = "yc.iam.openid-server-folder"
  }

  auth_ui_backend_group = {
    id = "ds7s0s5pprjhktjhn6q6"
  }

  iam_services_prod = {
    "oauth" = {
      virtual_host_name_suffix = "auth"
      authority                = [
        "auth.cloud.yandex.ru",
        "auth.cloud.yandex.com",
      ]
      target_group_id          = ycp_platform_alb_target_group.iam_openid_target_group.id
    }
  }

  iam_service_settings_prod = {
    for k, v in local.iam_services_prod : k => merge(
      lookup(module.defaults.iam_backends_defaults, k, {}),
      v
    )
  }
}
