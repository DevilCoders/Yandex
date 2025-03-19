package actions

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/adapters"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/services/env"
)

type baseAction struct {
	*env.Env
}

func (b *baseAction) mapRMError(ctx context.Context, cloudID string, err error) error {

	var statusErr interface {
		GRPCStatus() *status.Status
	}

	if !xerrors.As(err, &statusErr) {
		return err
	}

	switch {
	case statusErr.GRPCStatus().Code() == codes.NotFound:
		return ErrNotfoundCloudID{
			CloudID: cloudID,
		}
	case statusErr.GRPCStatus().Code() == codes.PermissionDenied:
		return ErrRMPermissionDenied.Wrap(err)
	default:
		return err
	}
}

func (b *baseAction) mapBillingError(ctx context.Context, cloudID string, err error) error {
	switch {
	case xerrors.Is(err, adapters.ErrNotFound):
		return ErrNotfoundCloudID{
			CloudID: cloudID,
		}
	case xerrors.Is(err, adapters.ErrBackendTimeout):
		return ErrBackendTimeout.Wrap(err)
	default:
		return err
	}
}
