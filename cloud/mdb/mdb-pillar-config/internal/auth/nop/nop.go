package nop

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/auth"
)

// Authenticator that does nothing.
// Remove after https://st.yandex-team.ru/ORION-109
type Authenticator struct{}

var _ auth.Authenticator = &Authenticator{}

func (a Authenticator) Authenticate(_ context.Context, _ []string) error {
	return nil
}
