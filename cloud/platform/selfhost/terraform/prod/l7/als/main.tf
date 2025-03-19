//     sa-ig = var.installation == "prod" ? "ajenj3j4dhm6caq2332m" : "bfboloihsdgbq10vi523"
module l7-als {
    source = "../../../modules/l7-als"
    image_id = "fd83il0sss2j95055rjl"
    installation = "prod"
    sa_ig = "ajenj3j4dhm6caq2332m"
    ig_size = 6
    sa_kms = "ajevuv4q7rflaqfdag3c"
    folder_id = "b1g2fr02n8tkm6pa6t39"
    subnets = {
        vla = "e9b9e47n23i7a9a6iha7"
        sas = "e2lt4ehf8hf49v67ubot"
        myt = "b0c7crr1buiddqjmuhn7"
    }
    user_data_path = "${path.module}/user-data.yaml"
}
