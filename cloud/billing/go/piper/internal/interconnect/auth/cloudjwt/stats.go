package cloudjwt

import (
	"context"
	"time"
)

type Stats struct {
	RefreshDate    time.Time
	ExpireDeadline time.Time
}

func (t *Authenticator) HealthCheck(_ context.Context) error {
	return t.refreshToken()
}

func (t *Authenticator) GetStats() Stats {
	return Stats{
		RefreshDate:    t.refreshDate,
		ExpireDeadline: t.expireDeadline,
	}
}
