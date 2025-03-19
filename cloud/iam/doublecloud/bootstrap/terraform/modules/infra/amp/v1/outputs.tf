output "workspace_id" {
  value = aws_prometheus_workspace.prom.id
}

output "region_id" {
  value = data.aws_region.current.name
}
