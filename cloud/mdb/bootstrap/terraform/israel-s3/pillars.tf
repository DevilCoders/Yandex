
resource "local_file" "pillars" {
  filename        = "${path.root}/../../../salt/pillar/mdb_s3_israel/generated.sls"
  file_permission = "0640"

  content = yamlencode({
    clusters = {
      zk01     = module.zk01
      s3meta01 = module.s3meta01
      pgmeta01 = module.pgmeta01
      s3db01   = module.s3db01
    }
  })
}
