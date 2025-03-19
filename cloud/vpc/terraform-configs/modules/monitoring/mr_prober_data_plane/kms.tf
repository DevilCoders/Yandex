resource "ycp_kms_symmetric_key" "mr_prober_secret_kek" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id = var.folder_id

  name = "mr-prober-secret-kek"
  description = "Key Encryption Key for SKM"

  default_algorithm = "AES_256"

  status = "ACTIVE"
}