resource "yandex_kms_symmetric_key" "k8sapi_kek" {
  name              = "k8s-api-kek"
  description       = "KEK for encrypting/decrypting k8s api secrets"
  default_algorithm = "AES_256"
}

resource "yandex_iam_service_account" "ig_sa" {
  name              = "ig-sa"
  description       = "Service account for managed k8s instance group management"
}
