resource "ycp_resource_manager_root_iam_binding" "backoffice_ui_sa_root_roles" {
  lifecycle {
    prevent_destroy = true
  }
  role    = "internal.oauth.client"
  members = [
    "serviceAccount:${ycp_iam_service_account.backoffice_ui_sa.id}"
  ]
}
