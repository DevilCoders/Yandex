// Service Cloud bindings https://st.yandex-team.ru/ORION-841
resource "ycp_resource_manager_cloud_iam_member" "billing_agent_admin_role" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id = ycp_resource_manager_cloud.billing_service_cloud.id
  role     = "admin"
  member   = "serviceAccount:${ycp_iam_service_account.billing_agent.id}"
}
// Root Bindings https://st.yandex-team.ru/ORION-851
resource "ycp_resource_manager_root_iam_binding" "billing_sa_agent_root" {
  lifecycle {
    prevent_destroy = true
  }
  for_each = toset([
    "internal.iam.userSettings.agent",
    "internal.billing.folderresolver",
    "internal.billing.cloudresolver",
    "internal.billing.accountResolver",
    "internal.sessionService.userAccountAgent",
    "internal.resource-manager.clouds.stageViewer"
  ])
  role = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.billing_agent.id}"
  ]
}
# ResourceType bindings  https://st.yandex-team.ru/ORION-851
resource "ycp_iam_resource_type_iam_binding" "billing_sa_agent_resource_type" {
  lifecycle {
    prevent_destroy = true
  }
  resource_type = "billing.account"
  role          = "internal.iam.accessBindings.admin"
  members = [
    "serviceAccount:${ycp_iam_service_account.billing_agent.id}",
  ]
}
