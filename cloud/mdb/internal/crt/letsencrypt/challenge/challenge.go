package challenge

import (
	"context"

	"golang.org/x/crypto/acme"
)

type ChallengeProvider interface {
	ChallengeType() string
	Prepare(ctx context.Context, acme *acme.Client, auth acme.Authorization, challenge acme.Challenge) error
	Cleanup(ctx context.Context, acme *acme.Client, auth acme.Authorization, challenge acme.Challenge) error
}
