variable "token" {
  description = "Iam token for stand"
}

locals {
  folder_id = "b1g2po48ce7bu9v9stf9" #enginfra 
}

output tfstate-sa-access-key{
  value = yandex_iam_service_account_static_access_key.tfstate-sa-static-key.access_key
}

output tfstate-sa-secret-key{
  value = yandex_iam_service_account_static_access_key.tfstate-sa-static-key.secret_key
}