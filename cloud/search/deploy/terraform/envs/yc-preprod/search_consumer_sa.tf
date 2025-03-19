resource "yandex_iam_service_account" "search-robot-consumer" {
  name        = "search-robot-consumer"
  description = "Service account to consume data from YC Logbroker"
}

resource "yandex_iam_service_account_key" "search-robot-consumer" {
  service_account_id = yandex_iam_service_account.search-robot-consumer.id
  description        = "key for service account"
  key_algorithm      = "RSA_2048"
}

output "search_robot_consumer_private_key" {
  value     = yandex_iam_service_account_key.search-robot-consumer.private_key
  sensitive = true
}

output "search_robot_consumer_public_key" {
  value     = yandex_iam_service_account_key.search-robot-consumer.public_key
  sensitive = true
}

output "search_robot_consumer_id" {
  value = yandex_iam_service_account_key.search-robot-consumer.id
}

output "search_robot_consumer_sa_id" {
  value = yandex_iam_service_account.search-robot-consumer.id
}
