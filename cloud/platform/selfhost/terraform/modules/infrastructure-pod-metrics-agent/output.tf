output "infra-pod-configs" {
  value = "${data.template_file.infra-configs.rendered}"
}

output "infra-pod-spec" {
  value = "${data.template_file.infra-pod-spec.rendered}"
}

output "infra-pod-configs-no-fluentd" {
  value = "${data.template_file.infra-configs-no-fluentd.rendered}"
}

output "infra-pod-spec-no-fluentd" {
  value = "${data.template_file.infra-pod-spec-no-fluentd.rendered}"
}

output "infra-pod-configs-no-push-client"{
  value = "${data.template_file.infra-configs-no-push-client.rendered}"
}

output "infra-pod-spec-no-push-client" {
  value = "${data.template_file.xds-infra-pod-spec-no-push-client.rendered}"
}

output "xds-infra-pod-configs-no-fluentd"{
  value = "${data.template_file.xds-infra-configs-no-fluentd.rendered}"
}

output "xds-infra-pod-spec-no-fluentd" {
  value = "${data.template_file.xds-infra-pod-spec-no-fluentd.rendered}"
}

output "mk8s-master-infra-pod-configs" {
  value = "${data.template_file.mk8s-master-infra-configs.rendered}"
}

output "mk8s-master-infra-pod-spec" {
  value = "${data.template_file.mk8s-master-infra-pod-spec.rendered}"
}
