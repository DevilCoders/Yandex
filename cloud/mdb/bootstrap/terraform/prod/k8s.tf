resource "ycp_iam_service_account" "k8s_cr_puller" {
  name               = "yc-k8s-cr-puller"
  service_account_id = "yc.mdb.k8s-cr-puller"
}

resource "yandex_container_registry_iam_binding" "puller" {
  registry_id = yandex_container_registry.registry.id
  role        = "container-registry.images.puller"

  members = [
    "serviceAccount:${ycp_iam_service_account.k8s_cr_puller.id}",
  ]
}
