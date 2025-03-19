resource "ycp_iam_service_account" "mr_prober_sa" {
  service_account_id = "yc.vpc.mr-prober-sa"
  folder_id = var.folder_id

  name = var.mr_prober_sa_name
  description = "Service account to manage Mr. Prober instances"
}

resource "ycp_resource_manager_folder_iam_member" "mr_prober_sa" {
  for_each = toset(["editor", "internal.computeadmin", "internal.compute.e2eplatformsuser"])

  folder_id = var.folder_id
  role = each.value
  member = "serviceAccount:${ycp_iam_service_account.mr_prober_sa.id}"
}

// Authorized key for the service account.
// Used for "ycp" terraform provider in cases where Control Plane's stand differs from
// Data Plane VMs stand.
//
// IMPORTANT: This secret should be uploaded to
// https://yav.yandex-team.ru/secret/sec-01ekye608rtvgts7nbazvg1fv9/explore/versions
// in JSON format after generating.
resource "ycp_iam_key" "mr_prober_sa" {
  key_id = "yc.vpc.mr-prober-sa.key"
  service_account_id = ycp_iam_service_account.mr_prober_sa.id
  description = "Authorized key (https://cloud.yandex.com/docs/iam/concepts/authorization/key) for ${var.mr_prober_sa_name}. Stored in YaV."
  key_algorithm = "RSA_4096"
  format = "PEM_FILE"
  public_key = var.mr_prober_sa_public_key
}
