resource "ycp_iam_service_account" "iam_ydb_migrate" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = var.iam_project_id
  service_account_id = "yc.iam.iamYdbMigrate"
  name               = "iam-ydb-migrate"
}

resource "ycp_iam_key" "iam_ydb_migrate" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.iam_ydb_migrate.id
  service_account_id = ycp_iam_service_account.iam_ydb_migrate.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA2Dd4Xc9kFZ60Io/FMp+J
HdlsSbcZFBsh+OsyqAgzG/pq4tNFYadDWJ27AGSx/atXhWyt61lze2Aq60vtPx8v
a202P7TRtrHiDH+pPAdGo2QWy3xOajvzCIJl7TTnfnLZrEfoCFgjY8Bv41bW8Waz
w3HEIt/GlHs6sAjK+QZ4rP7p+vd6pMcRUE0e4ahHR/mDZOJ9j6g9PhLGkYY7xNIj
nTkYZeLbCJqv8oWJoEPmfax/j4jFD0YMx2J+kxnmYTfs1XmqvbjvYhs8D2U1m5d3
quthIe7leCEhggg8oT4xyHX8nsSBCnAiHAyuN6Fg3vMxMICEV7Nseawd9oQlVysM
MBeWz7CNIo0acplN9HHO96YZZxaJivNrJAIykhszN6nw9ccwdrelqSJ6AmCDF8nH
G3K4p3Cf9JOaTOrnNW3DYk2aVEuGKeYV3SERpED0BDdA9J1HVcjGJ8WV731nbrTw
Wa2ZR+nmt9JkNpyA5Cdw5Biwo9svAV5uNRz4sSXVbyM/3JvNhPGLaqTEl1+MeAUZ
D5gPt433vFR2DYyn8llHZM0m2C3D/Rw0PeYQ1DttOpsogG/VwkIMI2M+oUCBbiUs
koWi+DFBMhRUmHu498yuctBkTy8H7nlgFZVuLH/A+3RJNFEFU9mKwfcpOTAxFbQy
9zx9F2WvVKgfBSast7M/J1cCAwEAAQ==
-----END PUBLIC KEY-----
EOF
}

resource "ycp_resource_manager_cloud_iam_member" "iam_ydb_migrate" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id = ycp_resource_manager_cloud.iam_service_cloud.id
  member   = "serviceAccount:${ycp_iam_service_account.iam_ydb_migrate.id}"
  role     = "admin"
}
