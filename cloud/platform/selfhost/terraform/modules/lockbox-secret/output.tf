output "id" {
    value = data.external.lockbox-secret-getter.result["id"]
}

output "version" {
    value = data.external.lockbox-secret-getter.result["version"]
}

output "secret" {
    value = data.external.lockbox-secret-getter.result["secret"]
}
