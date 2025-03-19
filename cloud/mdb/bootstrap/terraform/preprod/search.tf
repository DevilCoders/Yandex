resource "ycp_iam_service_account" "mdb-search-producer" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-search-producer"
  description        = "Sends search events"
  service_account_id = "yc.mdb.search-producer"
}

resource "ycp_iam_service_account" "mdb-search-reindexer" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-search-reindexer"
  description        = "Initiate reindex of clusters"
  service_account_id = "yc.mdb.search-reindexer"
}
