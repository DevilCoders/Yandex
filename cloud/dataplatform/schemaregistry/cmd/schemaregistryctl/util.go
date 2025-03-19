package main

import (
	"context"
	"crypto/tls"
	"fmt"
	"os"
	"strings"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
)

func grpcContext() context.Context {
	ctx := context.Background()
	if os.Getenv("SR_TOKEN") != "" {
		ctx = metadata.AppendToOutgoingContext(ctx, "Authorization", fmt.Sprintf("OAuth %s", os.Getenv("SR_TOKEN")))
	} else {
		logger.Log.Fatalf("unable to find schema registry token in ENV (SR_TOKEN)")
	}
	return ctx
}

func resolveConn(host string) (*grpc.ClientConn, error) {
	if strings.HasPrefix(host, "localhost") || os.Getenv("GRPC_PLAINTEXT") == "1" {
		conn, err := grpc.Dial(host, grpc.WithInsecure())
		if err != nil {
			return nil, err
		}
		return conn, err
	}
	conn, err := grpc.Dial(host, grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{InsecureSkipVerify: true})))
	if err != nil {
		return nil, err
	}
	return conn, err
}
