resource "yandex_iam_service_account" "s3fs" {
  name        = "s3fs-cloudbeaver"
  description = "s3fs s3-bucket access"
}

resource "yandex_resourcemanager_folder_iam_member" "s3fs_editor" {
  folder_id = var.folder_id
  member    = "serviceAccount:${yandex_iam_service_account.s3fs.id}"
  role      = "storage.editor"
}

resource "yandex_resourcemanager_folder_iam_member" "s3fs_kms_key" {
  folder_id = var.folder_id
  member    = "serviceAccount:${yandex_iam_service_account.s3fs.id}"
  role      = "kms.keys.encrypterDecrypter"
}

resource "yandex_iam_service_account_static_access_key" "s3fs_access_key" {
  service_account_id = yandex_iam_service_account.s3fs.id
  description        = "static access key for s3fs cloudbeaver"
}
