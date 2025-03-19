locals{
    module_path = path_relative_from_include()
    use_env_tools = get_env("USE_ENV_TOOLS", "0") != "0"
    cmd = local.use_env_tools ? "yt-oauth" : "${local.module_path}/modules/.bin/yt-oauth/yt-oauth"
}

inputs = {
    yt_token = run_cmd("--terragrunt-quiet", local.cmd)
}

generate "yt_token_vars"{
    path      = "generated.yt_tokens_var.tf"
    if_exists = "overwrite"
    contents = <<EOF
variable "yt_token" {
  description = "Yandex-Team security OAuth token"
  sensitive   = true
}
EOF
}
