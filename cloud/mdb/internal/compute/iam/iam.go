package iam

import (
	"context"
	"time"

	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh TokenService,CredentialsService,AbcService

var ErrNotFound = xerrors.NewSentinel("object not found")

// ServiceAccount defines service account credentials
type ServiceAccount struct {
	ID    string
	KeyID string
	Token []byte
}

type Token struct {
	Value     string
	ExpiresAt time.Time
}

type TokenService interface {
	TokenFromOauth(ctx context.Context, oauthToken string) (Token, error)
	ServiceAccountToken(ctx context.Context, serviceAccount ServiceAccount) (Token, error)
	ServiceAccountCredentials(serviceAccount ServiceAccount) CredentialsService
}

type CredentialsService interface {
	credentials.PerRPCCredentials
	Token(ctx context.Context) (string, error)
}

type ABC struct {
	CloudID         string
	AbcSlug         string
	AbcID           int64
	DefaultFolderID string
	AbcFolderID     string
}

type AbcService interface {
	ResolveByABCSlug(ctx context.Context, abcSlug string) (ABC, error)
	ResolveByCloudID(ctx context.Context, cloudID string) (ABC, error)
	ResolveByFolderID(ctx context.Context, folderID string) (ABC, error)
}
