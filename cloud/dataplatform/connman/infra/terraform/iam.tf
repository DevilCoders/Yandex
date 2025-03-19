resource "yandex_iam_service_account" "cm-admin" {
  name = "cm-admin"
}

resource "yandex_resourcemanager_folder_iam_member" "cm-admin-editor" {
  folder_id = data.yandex_resourcemanager_folder.default.id
  role      = "editor"
  member    = "serviceAccount:${yandex_iam_service_account.cm-admin.id}"
}

resource "yandex_resourcemanager_folder_iam_member" "cm-admin-puller" {
  folder_id = data.yandex_resourcemanager_folder.default.id
  role      = "container-registry.images.puller"
  member    = "serviceAccount:${yandex_iam_service_account.cm-admin.id}"
}
