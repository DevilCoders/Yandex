output "control_network_id" {
  value = ycp_vpc_network.mr_prober_control.id
}

locals {
  control_network_subnet_ids = {for subnet in ycp_vpc_subnet.mr_prober_control: subnet.zone_id => subnet.id}
}

output "control_network_subnet_ids" {
  value = local.control_network_subnet_ids
}

output "mr_prober_sa_id" {
  value = yandex_iam_service_account.mr_prober_sa.id

  depends_on = [
    ycp_resource_manager_folder_iam_member.mr_prober_sa
  ]
}

output "mr_prober_secret_kek_id" {
  value = yandex_kms_symmetric_key.mr_prober_secret_kek.id

  depends_on = [
    yandex_kms_symmetric_key.mr_prober_secret_kek
  ]
}
