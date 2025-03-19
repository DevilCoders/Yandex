package main

import (
	"context"
	"crypto/tls"
	"flag"
	"fmt"
	"os"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	folderID = flag.String("folder", "", "")
	name     = flag.String("name", "", "")
	cluterID = flag.String("cluster-id", "", "")
	user     = flag.String("user", "", "")
	password = flag.String("password", "", "")
)

func main() {
	iamToken := os.Getenv("IAM_TOKEN")
	md := metadata.New(map[string]string{
		"authorization": fmt.Sprintf("iam %v", iamToken),
	})
	ctx := metadata.NewOutgoingContext(context.Background(), md)

	creds := credentials.NewTLS(new(tls.Config))
	conn, err := grpc.Dial("grpc.connman.cloud-preprod.yandex.net:443", grpc.WithTransportCredentials(creds))
	if err != nil {
		logger.Log.Fatal("unable to dial", log.Error(err))
	}
	defer func(conn *grpc.ClientConn) {
		err := conn.Close()
		if err != nil {
			logger.Log.Error("unable to close connection", log.Error(err))
		}
	}(conn)

	client := connman.NewConnectionServiceClient(conn)
	op, err := client.Create(ctx, &connman.CreateConnectionRequest{
		FolderId:    *folderID,
		Name:        *name,
		Description: "auto created by go-producer",
		Labels:      map[string]string{"example-key": "example-value"},
		Params: &connman.ConnectionParams{Type: &connman.ConnectionParams_Clickhouse{Clickhouse: &connman.ClickHouseConnection{
			Connection: &connman.ClickHouseConnection_ManagedClusterId{ManagedClusterId: *cluterID},
			Auth: &connman.ClickHouseAuth{Security: &connman.ClickHouseAuth_UserPassword{UserPassword: &connman.UserPasswordAuth{
				User:     *user,
				Password: &connman.Password{Value: &connman.Password_Raw{Raw: *password}},
			}}},
		}}},
	})
	if err != nil {
		logger.Log.Fatal("unable to dial", log.Error(err))
	}
	connection := new(connman.Connection)
	if err := op.GetResponse().UnmarshalTo(connection); err != nil {
		logger.Log.Fatal("unable to unmarshal connection", log.Error(err))
	}
	logger.Log.Infof("connection: %v", connection)
}
