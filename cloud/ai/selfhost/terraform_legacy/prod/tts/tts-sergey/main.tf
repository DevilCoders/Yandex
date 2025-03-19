module "ai_tts_service_instance_group" {

    source = "../base_np"
    yandex_token = var.yandex_token
    name = "ai-tts-service-sergey"

    yc_instance_group_size = var.yc_instance_group_size

    max_unavailable = var.max_unavailable
    max_creating = var.max_creating
    max_expansion = var.max_expansion
    max_deleting = var.max_deleting
    yc_zones = var.yc_zones

    tts_server_version = "tts-server:multilang-cloud-general_trt7_a100_9739554"

#    instance_gpus = 1
#    instance_cores = 8
#    instance_memory = 96
#    instance_max_memory = 88
    instance_gpus   = 1
    instance_cores  = 28
    instance_memory = 119
    instance_max_memory = 80
    image_id = "fd8c0blbv50bha95cmt2"
    #platform_id = "gpu-private-v3"
}
