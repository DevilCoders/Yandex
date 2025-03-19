resource "ycp_resource_manager_folder_iam_member" "iam_service_sa_ydb_viewer" {
  for_each = toset(local.iam_service_account_ids)

  lifecycle {
    prevent_destroy = true
  }
  folder_id = ycp_resource_manager_folder.iam_service_folder.id
  member    = format("serviceAccount:%s", each.key)
  role      = "ydb.viewer"
}

resource "ycp_resource_manager_folder_iam_member" "iam_service_sa_ydb_editor" {
  for_each = toset(local.iam_service_account_ids)

  lifecycle {
    prevent_destroy = true
  }
  folder_id = ycp_resource_manager_folder.iam_service_folder.id
  member    = format("serviceAccount:%s", each.key)
  role      = "ydb.editor"
}

resource "ycp_resource_manager_cloud_iam_member" "iam_service_cloud_ydb_admin_iam_service_sa" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id = ycp_resource_manager_cloud.iam_service_cloud.id
  member   = "serviceAccount:${ycp_iam_service_account.iam_service_account.id}"
  role     = "ydb.admin"
}
