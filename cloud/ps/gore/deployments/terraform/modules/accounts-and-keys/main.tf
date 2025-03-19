resource "yandex_kms_symmetric_key" "secret_encryptor" {
  lifecycle {
    prevent_destroy = true
  }

  name              = "secret_encryptor"
  description       = "Ключ для шифрования секретов, которые доставляются на виртуальные машины"
  default_algorithm = "AES_256"
}

resource "yandex_iam_service_account" "gore" {
  name        = "gore-sa"
  description = "Сервисный аккаунт для виртуальных машин"
  folder_id   = var.folder_id
}

resource "yandex_resourcemanager_folder_iam_binding" "gore" {
  folder_id = var.folder_id
  role = "kms.keys.encrypterDecrypter"

  members = [
    "serviceAccount:${yandex_iam_service_account.gore.id}"
  ]
}
