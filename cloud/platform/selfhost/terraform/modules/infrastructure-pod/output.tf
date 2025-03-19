output "infra-pod-configs" {
  value = "${data.template_file.infra-configs.rendered}"
}

output "infra-pod-spec" {
  value = "${data.template_file.infra-pod-spec.rendered}"
}

output "infra-pod-configs-no-push-client" {
  value = "${data.template_file.infra-configs-no-push-client.rendered}"
}

output "infra-pod-spec-no-push-client" {
  value = "${data.template_file.infra-pod-spec-no-push-client.rendered}"
}

output "infra-pod-configs-no-push-client-no-solomon" {
  value = "${data.template_file.infra-configs-no-push-client-no-solomon.rendered}"
}

output "infra-pod-spec-no-push-client-no-solomon" {
  value = "${data.template_file.infra-pod-spec-no-push-client-no-solomon.rendered}"
}
