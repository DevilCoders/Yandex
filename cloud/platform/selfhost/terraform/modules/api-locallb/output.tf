output "api_configs_metadata" {
  value = data.template_file.api_configs.rendered
}

output "api_podmanifest" {
  value = data.template_file.api_podmanifest.rendered
}
