output "podmanifest" {
  value = local.podmanifest
}

output "podmanifest_rendered" {
  value = yamlencode(local.podmanifest)
}