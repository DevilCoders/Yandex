package auth

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth/permissions"
)

type AuthBackend interface {
	AuthorizeBillingAdmin(ctx context.Context, perm permissions.Permission) error
	AuthorizeBillingAdminWithToken(ctx context.Context, token string, perm permissions.Permission) error
}
