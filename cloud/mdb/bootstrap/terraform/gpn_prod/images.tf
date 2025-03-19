resource "ycp_iam_service_account" "image-releaser" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = var.clusters_folder
  name               = "image-releaser"
  description        = "account to release images for clusters"
  service_account_id = "yc.mdb.image-releaser"
}
