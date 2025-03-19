// We create container registry in PROD only and use cr.yandex in all stands and environments.

resource "yandex_container_registry" "gore" {
  name      = "gore"
  folder_id = "yc.gore.service-folder"
}

resource "yandex_container_registry" "dutybot" {
  name      = "dutybot"
  folder_id = "yc.gore.service-folder"
}

resource "yandex_resourcemanager_folder_iam_binding" "puller" {
  folder_id = "yc.gore.service-folder"
  role      = "container-registry.images.puller"

  members = [
    "serviceAccount:${module.accounts_and_keys.gore_sa_id}"
  ]
}

