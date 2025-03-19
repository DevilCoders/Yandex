output "secret_encryptor_key_id" {
  value = yandex_kms_symmetric_key.secret_encryptor.id
}

output "gore_sa_id" {
  value = yandex_iam_service_account.gore.id
}
