variable "dogfood_project_id" {
  type = string
}

variable "dogfood_project_name" {
  type = string
}

resource "ycp_resource_manager_cloud" "mdb_junk_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = var.dogfood_project_id
  name            = var.dogfood_project_name
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "mdb_junk_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.mdb_junk_cloud.id
  folder_id = var.dogfood_project_id
  name      = var.dogfood_project_name
}
