module "defaults" {
  source = "../modules/constants/defaults"
}

locals {
  openid_folder = {
    id = "yc.iam.openid-server-folder"
  }

  ###################################################################################
  ##################################### PREPROD #####################################
  ###################################################################################

  auth_ui_backend_group = {
    id = "a5dvbp461qpv06blch3j"
  }

  iam_services_preprod = {
    "oauth" = {
      virtual_host_name_suffix = "auth"
      authority                = [
        "auth-preprod.cloud.yandex.ru",
        "auth-preprod.cloud.yandex.com",
      ]
      target_group_id          = ycp_platform_alb_target_group.iam_openid_target_group_preprod.id
    }
  }

  iam_service_settings_preprod = {
    for k, v in local.iam_services_preprod : k => merge(
      lookup(module.defaults.iam_backends_defaults, k, {}),
      v
    )
  }

  ###################################################################################
  ##################################### TESTING #####################################
  ###################################################################################

  iam_services_testing = {
    "oauth" = {
      virtual_host_name_suffix = "auth"
      authority                = [
        "auth-testing.cloud.yandex.ru",
        "auth-testing.cloud.yandex.com",
      ]
      target_group_id          = ycp_platform_alb_target_group.iam_openid_target_group_testing.id
    }
  }

  iam_service_settings_testing = {
    for k, v in local.iam_services_testing : k => merge(
      lookup(module.defaults.iam_backends_defaults, k, {}),
      v
    )
  }
}
