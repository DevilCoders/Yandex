output "id" {
  value = data.external.yav-secret-getter.result.id
}

output "verison" {
  value = data.external.yav-secret-getter.result.version
}

output "secret" {
  value = data.external.yav-secret-getter.result.secret
}
