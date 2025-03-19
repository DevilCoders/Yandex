resource "ycp_iam_service_account" "gsoc_robot_sa" {
  name              = "gsoc-robot-sa"
  description       = "Service Account for gsoc-robot instances"
  folder_id         = var.yc_folder
}

resource "ycp_resource_manager_folder_iam_member" "gsoc_robot_sa_editor" {
  folder_id = var.yc_folder
  role      = "kms.keys.encrypterDecrypter"
  member    = "serviceAccount:${ycp_iam_service_account.gsoc_robot_sa.id}"
}