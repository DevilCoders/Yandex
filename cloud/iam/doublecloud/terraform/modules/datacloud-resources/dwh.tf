locals {
  dwh_service_project_id = "yc.dwh.service"
}

resource "ycp_resource_manager_cloud" "dwh_service_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.dwh_service_project_id
  name            = "dwh-service-cloud"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "dwh_service_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.dwh_service_cloud.id
  folder_id = local.dwh_service_project_id
  name      = "dwh-service-folder"
}
