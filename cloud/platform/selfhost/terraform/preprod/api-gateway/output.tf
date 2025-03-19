output "blue" {
    value = {
        instance_group_id = module.group_blue.instance_group_id
        target_group_id   = module.group_blue.target_group_id
    }
}

output "green" {
    value = {
        instance_group_id = module.group_green.instance_group_id
        target_group_id   = module.group_green.target_group_id
    }
}

output "l7" {
    value = {
        main_group   = local.main_backend.group.name
        prev_group   = local.prev_backend.group.name
        canary_group = local.canary_backend.group.name
    }
}
