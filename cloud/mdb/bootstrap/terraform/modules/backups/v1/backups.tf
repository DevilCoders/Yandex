resource "ycp_resource_manager_folder" "mdb-backups" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = var.cloud_id
  folder_id = "yc.mdb.backups"
  name      = "backups"
}


resource "ycp_iam_service_account" "backups-admin" {
  lifecycle {
    prevent_destroy = true
  }
  service_account_id = "yc.mdb.backups-admin"
  name               = "backups-admin"
  description        = "services account for buckets management"
  folder_id          = ycp_resource_manager_folder.mdb-backups.folder_id
}

resource "ycp_resource_manager_folder_iam_member" "backups-admin-sa-admin" {
  folder_id = ycp_resource_manager_folder.mdb-backups.folder_id
  role      = "iam.serviceAccounts.admin"
  member    = "serviceAccount:${ycp_iam_service_account.backups-admin.id}"
}

resource "ycp_resource_manager_folder_iam_member" "backups-dmin-storage-admin" {
  folder_id = ycp_resource_manager_folder.mdb-backups.folder_id
  role      = "storage.admin"
  member    = "serviceAccount:${ycp_iam_service_account.backups-admin.id}"
}

resource "yandex_iam_service_account_key" "backups-admin-auth-key" {
  service_account_id = ycp_iam_service_account.backups-admin.id
  description        = "key for creating service-accounts, used in dbaas_worker"
  key_algorithm      = "RSA_4096"
}

resource "yandex_iam_service_account_static_access_key" "backups-admin-static-key" {
  service_account_id = ycp_iam_service_account.backups-admin.id
  description        = "key for accessing s3 storage, used in dbaas_worker and mdb-internal-api"
}