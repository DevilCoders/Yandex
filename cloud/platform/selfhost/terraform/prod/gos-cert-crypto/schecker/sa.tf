resource "ycp_iam_service_account" "schecker_sa" {
  name              = "schecker-sa"
  description       = "Service Account for schecker instances"
  folder_id         = var.yc_folder
}

resource "ycp_resource_manager_folder_iam_member" "schecker_sa_kms_encrypter" {
  folder_id = var.yc_folder
  role      = "kms.keys.encrypterDecrypter"
  member    = "serviceAccount:${ycp_iam_service_account.schecker_sa.id}"
}