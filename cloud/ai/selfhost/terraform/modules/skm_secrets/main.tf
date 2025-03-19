locals {
  files    = [for f in var.file_secrets : { path : f.path, content : f.content, source_path : "${abspath(path.root)}/.terraform/tmp/${replace(f.path, "/", ".")}.txt" }]
  profile  = var.environment == "preprod" ? "preprod-fed" : "prod-fed"
  endpoint = var.environment == "preprod" ? "api.cloud-preprod.yandex.net:443" : "api.cloud.yandex.net:443"
}

data "external" "skm_runner" {
  depends_on = []

  program = [
    "python3",
  "${path.module}/skm_runner.py"]

  query = {
    profile   = local.profile
    skm_path  = var.skm_path
    yav_token = var.yav_token
    files     = jsonencode(local.files)
    src = templatefile("${path.module}/files/skm-encrypt.tpl.yaml", {
      endpoint      = local.endpoint
      kms_key_id    = var.kms_key_id
      encrypted_dek = var.encrypted_dek
      secrets       = var.yav_secrets
      files         = local.files
    })
  }
}