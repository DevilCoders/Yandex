package iam

import "context"

//go:generate mockery --name RMClient

type RMClient interface {
	ResolveFolder(ctx context.Context, folderIDs ...string) (result []ResolvedFolder, err error)
	HealthCheck(context.Context) error
}
