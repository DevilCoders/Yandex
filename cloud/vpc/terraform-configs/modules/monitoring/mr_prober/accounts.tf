resource "yandex_iam_service_account" "mr_prober_sa" {
  folder_id = var.folder_id

  name = var.mr_prober_sa_name
  description = "Service account to manage Mr. Prober instances"
}

resource "ycp_resource_manager_folder_iam_member" "mr_prober_sa" {
  for_each = toset([
    "editor",
    "internal.computeadmin",
    "internal.compute.e2eplatformsuser",
    // storage.admin is required for creating/modifying buckets via terraform:
    // https://st.yandex-team.ru/CLOUDSUPPORT-146390,
    "storage.admin"
  ])

  folder_id = var.folder_id
  role = each.value
  member = "serviceAccount:${yandex_iam_service_account.mr_prober_sa.id}"
}

// Authorized key for the service account. 
//
// IMPORTANT: This secret should be uploaded to
// https://yav.yandex-team.ru/secret/sec-01ekye608rtvgts7nbazvg1fv9/explore/versions
// in JSON format after generating.
resource "yandex_iam_service_account_key" "mr_prober_sa" {
  service_account_id = yandex_iam_service_account.mr_prober_sa.id
  description = "Authorized key (https://cloud.yandex.com/docs/iam/concepts/authorization/key) for ${var.mr_prober_sa_name}. Stored in YaV and Sandbox Vault."
  key_algorithm = "RSA_4096"
}
