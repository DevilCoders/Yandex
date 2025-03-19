resource "ycp_resource_manager_root_iam_binding" "datalens_ui_notify_sa_root_roles" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.backoffice.notify.agent",
    "internal.iam.userSettings.agent",
  ])
  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.datalens_ui_notify_sa.id}"
  ]
}

# Waiting for merge `datalens.dcWorkbook` resource – https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/pull-requests/20233/overview
# ResourceType bindings https://st.yandex-team.ru/DLPROJECTS-9
# resource "ycp_iam_resource_type_iam_binding" "datalens_abs_sa_resource_type" {
#   lifecycle {
#     prevent_destroy = true
#   }
#   resource_type = "datalens.dcWorkbook"
#   role          = "internal.iam.accessBindings.admin"
#   members = [
#     "serviceAccount:${ycp_iam_service_account.datalens_abs_sa.id}",
#   ]
# }
