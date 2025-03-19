# ROOT bindings
resource "ycp_resource_manager_root_iam_binding" "ydb_admin_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.ydb.controlPlaneAgent",
  ])
  role     = each.key
  members  = [
    "serviceAccount:${ycp_iam_service_account.ydb_admin_sa.id}"
  ]
}

resource "ycp_resource_manager_root_iam_binding" "ydb_viewer_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "ydb.viewer",
  ])
  role     = each.key
  members  = [
    "serviceAccount:${ycp_iam_service_account.ydb_viewer_sa.id}"
  ]
}

resource "ycp_resource_manager_root_iam_binding" "yds_kinesis_proxy_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.ydb.serverlessAgent",
    "iam.serviceAccounts.tokenCreator",
  ])
  role     = each.key
  members  = [
    "serviceAccount:${ycp_iam_service_account.yds_kinesis_proxy_sa.id}"
  ]
}

resource "ycp_resource_manager_root_iam_binding" "ydb_controlplane_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "ydb.editor",
  ])
  role     = each.key
  members  = [
    "serviceAccount:${ycp_iam_service_account.ydb_controlplane_sa.id}"
  ]
}

# CLOUD bindings
resource "ycp_resource_manager_cloud_iam_member" "ydb_service_cloud_ydb_admin_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "admin",
  ])

  cloud_id = ycp_resource_manager_cloud.ydb_service_cloud.id
  role     = each.key
  member   = "serviceAccount:${ycp_iam_service_account.ydb_admin_sa.id}"
}
