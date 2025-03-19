resource "yandex_kms_symmetric_key" "cm-secret-key" {
  lifecycle {
    prevent_destroy = true
  }
  name              = "cm-secret-key"
  default_algorithm = "AES_256"
}
