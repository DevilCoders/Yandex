package main

import (
	"context"
	"log"
	"os"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
)

const (
	address = "billing.private-api.cloud-preprod.yandex.net:16466"
)

func main() {
	ctx := context.Background()

	opts := make([]grpc.DialOption, 0)
	opts = append(opts, grpc.WithBlock())
	opts = append(opts, grpc.WithDisableRetry())

	if len(os.Args) > 1 {
		creds, err := credentials.NewClientTLSFromFile(os.Args[1], "")
		if err != nil {
			log.Fatalf("cert %v", err)
		}
		opts = append(opts, grpc.WithTransportCredentials(creds))
	} else {
		opts = append(opts, grpc.WithInsecure())
	}

	conn, err := grpc.DialContext(ctx, address, opts...)
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	skuService := billing_grpc.NewSkuServiceClient(conn)

	r, err := skuService.List(ctx, &billing_grpc.ListSkusRequest{})
	if err != nil {
		log.Fatalf("could not interact: %v", err)
	}
	log.Printf("Got response: %s", r.GetSkus())
}
