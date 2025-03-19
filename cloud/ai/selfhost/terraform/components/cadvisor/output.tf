output "container" {
  description = "Resulting container to be rendered into podmanifest"
  value       = module.cadvisor_container.container
}

output "configs" {
  description = "Component configs to be used by configs deployer"
  value       = {}
}

output "volumes" {
  description = "Volumes to be created on the host system (part of podmanifest)"
  value       = {}
}

output "envvar" {
  description = "Self produced environment variables"
  value       = {}
}

