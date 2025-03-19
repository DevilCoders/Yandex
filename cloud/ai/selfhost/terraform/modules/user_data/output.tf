output "rendered" {
  value = data.template_file.user_data.rendered
  #value = yamlencode(local.user_data)
}