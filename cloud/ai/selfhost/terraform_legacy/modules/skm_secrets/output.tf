output "path" {
    value = {for key, value in var.yav_secrets: key => value.path}
}

output "bundle" {
    value = data.external.skm_runner.result.dst
}
