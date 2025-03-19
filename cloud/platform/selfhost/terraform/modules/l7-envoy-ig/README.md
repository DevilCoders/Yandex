# Usage

```
module "ig" {
  source = "../../../modules/l7-envoy-ig"
  module_depends_on = [
    ... any deps, for example
    null_resource.server_cert.id,
  ]

  name = "ig-name"
  ...
}

output "ig" {
  value = <<EOT
    Instace group:
    IG: ${module.ig.instance_group_id}
    TG: ${module.ig.target_group_id}
  EOT
}
```

# Import

```
terraform import module.ig.ycp_microcosm_instance_group_instance_group.this cl1p397q5md3g4a79e3j
```
