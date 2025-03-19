inputs = {
    yc_token = run_cmd("--terragrunt-quiet", "yc", "iam", "create-token")
}

generate "yc_token_vars"{
    path      = "generated.yc_tokens_var.tf"
    if_exists = "overwrite"
    contents = <<EOF
variable "yc_token" {
  description = "Yandex Cloud IAM token"
  sensitive   = true
}
EOF
}
