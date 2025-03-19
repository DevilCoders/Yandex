resource "ycp_resource_manager_folder" "folder" {
  count = var.create_folder ? 1 : 0
  lifecycle {
    prevent_destroy = true
  }
  cloud_id    = var.cloud_id
  folder_id   = var.folder_id
  name        = var.prefix
  description = "Folder for cluster ${var.prefix}"
}