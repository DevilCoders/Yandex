# Example of switching green->blue
# Initial state
# canary_backend.group = local.green_group
# main_backend.group   = local.green_group
# prev_backend.group   = local.blue_group
# main_backend.weight = 100
# prev_backend.weight = -1
#
# 1. Remove blue from balancer and apply
# prev_backend.group   = local.green_group
#
# 2. Update blue and apply
# blue_image_id = "new-image-id"
#
# 3. Add blue to canary and apply
# canary_backend.group = local.blue_group
#
# 4. Run NewPreProdCanary test
#
# 5. Switch main and apply
# main_backend.group   = local.blue_group
# prev_backend.group   = local.green_group
# main_backend.weight = -1
# prev_backend.weight = 100
#
# 6. Change weights 10/90, 50/50, 100,-1

locals {
    blue_image_id = "fdv06lmjdbafrs3e0gv5"
    green_image_id = "fdvq8nt0k35nmm77ks1h" # latest

    // api.canary.ycp.cloud-preprod.yandex.net"
    canary_backend = {
        group = local.green_group
    }
    // A part of api.cloud-preprod.yandex.net according to weight
    main_backend = {
        group = local.green_group
        weight = 100
    }
    // api.ycp.cloud-preprod.yandex.net
    // A part of api.cloud-preprod.yandex.net according to weight
    prev_backend = {
        group = local.blue_group
        weight = -1
    }
}
