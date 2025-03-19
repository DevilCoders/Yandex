data "external" "yav-token" {
  program = ["${path.module}/get-token.sh"]
}

output "token" {
  value = data.external.yav-token.result.token
}
