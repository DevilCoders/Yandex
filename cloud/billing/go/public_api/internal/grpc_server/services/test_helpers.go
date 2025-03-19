package services

import (
	"context"

	"github.com/gofrs/uuid"
	"google.golang.org/grpc/metadata"
)

func generateID() string {
	return uuid.Must(uuid.NewV4()).String()
}

func getContextWithAuth() context.Context {
	md := metadata.New(map[string]string{"authorization": "Bearer " + generateID()})
	return metadata.NewIncomingContext(context.Background(), md)
}
