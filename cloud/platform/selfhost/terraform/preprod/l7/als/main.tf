//     sa-ig = var.installation == "prod" ? "ajenj3j4dhm6caq2332m" : "bfboloihsdgbq10vi523"
module l7-als {
    source = "../../../modules/l7-als"
    image_id = "fdvogo161u0gchoeb1hk"
    installation = "preprod"
    sa_ig = "bfboloihsdgbq10vi523"
    ig_size = 6
    core_fraction = 50
    sa_kms = "bfbnfajrg6lo05i4ech2"
    folder_id = "aoegudf63bpvp5b8f3g7"
    subnets = {
        vla = "bucpba0hulgrkgpd58qp"
        sas = "bltueujt22oqg5fod2se"
        myt = "fo27jfhs8sfn4u51ak2s"
    }
}
