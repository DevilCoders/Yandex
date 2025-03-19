module "constants" {
  source = "../modules/constants"
}

module "system_resources" {
  source = "../modules/system-resources"

  iam_project_id = module.constants.iam_project_id

  access_service_public_key    = local.access_service_public_key
  iam_control_plane_public_key = local.iam_control_plane_public_key
  mfa_service_public_key       = local.mfa_service_public_key
  openid_server_public_key     = local.openid_server_public_key
  org_service_public_key       = local.org_service_public_key
  reaper_public_key            = local.reaper_public_key
  rm_control_plane_public_key  = local.rm_control_plane_public_key
  token_service_public_key     = local.token_service_public_key

  iam_sync_public_key          = local.iam_sync_public_key
  iam_common_sa_public_key     = local.iam_common_sa_public_key
}

module "datacloud_resources" {
  source                      = "../modules/datacloud-resources"

  dogfood_project_id             = local.dogfood_project_id
  dogfood_project_name           = local.dogfood_project_name

  backoffice_ui_sa_public_key    = local.backoffice_ui_sa_public_key
  billing_agent_public_key       = local.billing_agent_public_key
  data_transfer_agent_public_key = local.data_transfer_agent_public_key
  datalens_back_file_upl_pub_key = local.datalens_back_file_upl_pub_key
  datalens_ui_public_key         = local.datalens_ui_public_key
  datalens_ui_notify_public_key  = local.datalens_ui_notify_public_key
  datalens_abs_public_key        = local.datalens_abs_public_key
  mdb_admin_public_key           = local.mdb_admin_public_key
  mdb_billing_public_key         = local.mdb_billing_public_key
  mdb_e2e_tests_public_key       = local.mdb_e2e_tests_public_key
  mdb_internal_api_public_key    = local.mdb_internal_api_public_key
  mdb_backstage_public_key       = local.mdb_backstage_public_key
  mdb_worker_public_key          = local.mdb_worker_public_key
  mdb_reaper_public_key          = local.mdb_reaper_public_key
  oauth_ssoui_public_key         = local.oauth_ssoui_public_key
  ydb_admin_sa_public_key        = local.ydb_admin_sa_public_key
  ydb_viewer_sa_public_key       = local.ydb_viewer_sa_public_key
  ydb_controlplane_sa_public_key = local.ydb_controlplane_sa_public_key
  yds_kinesis_proxy_sa_public_key = local.yds_kinesis_proxy_sa_public_key
}
