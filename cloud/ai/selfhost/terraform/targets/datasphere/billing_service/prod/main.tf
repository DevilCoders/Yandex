module "ds_billing_service_prod" {
  source = "../common"

  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  environment        = "prod"
  folder_id          = "b1g4ujiu1dq5hh9544r6"
  service_account_id = "ajeicsp8o15vdnjd5sv1"
  tls_certificate_id = "fpql1b9msi4nsmsjkjtl"

  s3_access_key = var.s3_access_key
  s3_secret_key = var.s3_secret_key

  db_password   = var.db_password

  tvm_secret    = var.tvm_secret
}
