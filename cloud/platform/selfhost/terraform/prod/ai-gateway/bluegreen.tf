locals {
    blue_image_id = "fd8bmuifpum8j3b1ejpe"
    green_image_id = "fd8k7nsnu2af3mnksukl" # latest

    main_backend = {
        group = local.green_group
        weight = 100
    }
    prev_backend = {
        group = local.blue_group
        weight = -1
    }
}
