resource "yandex_kms_symmetric_key" "mk8s_kek" {
  name              = "mk8s-controller-kek"
  description       = "KEK for encrypting/decrypting k8s controllers' secrets"
  default_algorithm = "AES_256"
}
