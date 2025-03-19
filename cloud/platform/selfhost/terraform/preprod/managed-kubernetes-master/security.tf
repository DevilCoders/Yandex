resource "yandex_kms_symmetric_key" "apiserver_kek" {
  name              = "apiserver-kek"
  description       = "KEK for encrypting/decrypting backend apiserver secrets"
  default_algorithm = "AES_256"
}

resource "yandex_iam_service_account" "apiserver_instance_sa" {
  name              = "apiserver-instance-sa"
  description       = "Service Account for apiserver instances"
}
