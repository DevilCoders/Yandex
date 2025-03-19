resource "ycp_iam_service_account" "ingress" {
  name               = "yc-internal-mdb-dev-ingress"
  service_account_id = "yc.mdb.internal-mdb-dev-ingress"
}

resource "yandex_iam_service_account_key" "ingress_key" {
  service_account_id = ycp_iam_service_account.ingress.service_account_id
}

resource "yandex_resourcemanager_folder_iam_member" "k8s_folder_editor" {
  folder_id = var.folder_id
  member    = "serviceAccount:${ycp_iam_service_account.ingress.id}"
  role      = "editor"
}

resource "yandex_resourcemanager_folder_iam_member" "k8s_cert_manager" {
  folder_id = var.folder_id
  member    = "serviceAccount:${ycp_iam_service_account.ingress.id}"
  role      = "certificate-manager.certificates.downloader"
}

resource "kubernetes_namespace" "namespace" {
  metadata {
    name = "yc-alb-ingress"
  }
}
