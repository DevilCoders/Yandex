package dummy

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth/permissions"
)

type dummy struct{}

func NewDummyAuthBackend() dummy {
	return dummy{}
}

func (dummy) AuthorizeBillingAdmin(ctx context.Context, perm permissions.Permission) error {
	return nil
}

func (dummy) AuthorizeBillingAdminWithToken(ctx context.Context, token string, perm permissions.Permission) error {
	return nil
}
