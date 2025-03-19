output "container" {
  description = "Resulting container to be rendered into podmanifest"
  value       = module.unified_agent_container.container
}

output "configs" {
  description = "Component configs to be used by configs deployer"
  value       = local.configs
}

output "volumes" {
  description = "Volumes to be created on the host system (part of podmanifest)"
  value       = {
    unified-agent-config = {
      hostPath = {
        path = "/etc/yandex/unified_agent"
        type = "DirectoryOrCreate"
      }
    }
  }
}

output "envvar" {
  description = "Self produced environment variables"
  value       = {
    TVM_SECRET = var.tvm_secret
  }
}