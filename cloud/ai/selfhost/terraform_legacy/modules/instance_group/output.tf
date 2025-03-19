output "group_id" {
  value = yandex_compute_instance_group.group.id
}

output "group_created_at" {
  value = [
    yandex_compute_instance_group.group.created_at,
  ]
}
