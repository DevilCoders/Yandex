resource "yandex_kms_symmetric_key" "etcd_kek" {
  name              = "etcd-kek"
  description       = "KEK for encrypting/decrypting backend etcd secrets"
  default_algorithm = "AES_256"
}

resource "yandex_iam_service_account" "etcd_instance_sa" {
  name              = "etcd-instance-sa"
  description       = "Service Account for etcd instances"
}
