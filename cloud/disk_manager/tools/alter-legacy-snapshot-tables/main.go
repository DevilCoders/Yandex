package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"os"
	"path"

	ydb "github.com/ydb-platform/ydb-go-sdk/v3"
	ydb_options "github.com/ydb-platform/ydb-go-sdk/v3/table/options"
)

////////////////////////////////////////////////////////////////////////////////

func main() {
	endpoint := flag.String("endpoint", "", "ydb endpoint")
	database := flag.String("database", "", "ydb database")
	tablesPath := flag.String("path", "", "ydb tables path")
	blobsPool := flag.String("blobs-pool", "rotencrypted", "external blobs pool")
	flag.Parse()

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	connectString := fmt.Sprintf("%v/?database=%v", *endpoint, *database)
	db, err := ydb.Open(
		ctx,
		connectString,
		ydb.WithAccessTokenCredentials(os.Getenv("YDB_TOKEN")),
	)
	if err != nil {
		log.Fatalf("connect error: %v", err)
	}
	defer db.Close(ctx)

	session, err := db.Table().CreateSession(ctx)
	if err != nil {
		log.Fatalf("create session error: %v", err)
	}
	defer session.Close(ctx)

	err = session.AlterTable(ctx, path.Join(*tablesPath, "blobs_on_hdd"),
		ydb_options.WithAlterStorageSettings(ydb_options.StorageSettings{
			External:           ydb_options.StoragePool{Media: *blobsPool},
			StoreExternalBlobs: ydb_options.FeatureEnabled,
		}),
	)
	if err != nil {
		log.Fatalf("failed to alter table 'blobs_on_hdd': %v", err)
	}
	log.Print("altered table 'blobs_on_hdd'")
}
