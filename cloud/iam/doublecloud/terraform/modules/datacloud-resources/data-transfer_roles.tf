# ROOT bindings
resource "ycp_resource_manager_root_iam_binding" "data_transfer_agent_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "mdb.viewer",
    "dcnetwork.viewer",
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.data_transfer_agent_sa.id}"
  ]
}
