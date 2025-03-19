output "id" {
  value = data.external.yav_secret_getter.result.id
}

output "version" {
  value = data.external.yav_secret_getter.result.version
}

output "secret" {
  value = data.external.yav_secret_getter.result.secret
}
