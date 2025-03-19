

resource "yandex_iam_service_account" "tfstate-sa" {
  name        = "tfstate-sa"
  description = "SA to manage terrafrom state in yc-devel/enginfra S3 storage"
}

resource "yandex_iam_service_account_static_access_key" "tfstate-sa-static-key" {
  service_account_id = yandex_iam_service_account.tfstate-sa.id
  description        = "static access key for object storage"
}

// Grant permissions 
resource "yandex_resourcemanager_folder_iam_member" "sa-editor" {
  folder_id = local.folder_id
  role      = "storage.editor"
  member    = "serviceAccount:${yandex_iam_service_account.tfstate-sa.id}"
}

resource "yandex_storage_bucket" "yc-devel-enginfra-tfstate" {
  bucket = "yc-devel-enginfra-tfstate"
  access_key = yandex_iam_service_account_static_access_key.tfstate-sa-static-key.access_key
  secret_key = yandex_iam_service_account_static_access_key.tfstate-sa-static-key.secret_key
}

