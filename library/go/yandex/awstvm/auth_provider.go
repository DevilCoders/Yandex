package awstvm

import (
	"context"
	"errors"
	"fmt"

	"github.com/aws/aws-sdk-go/aws/credentials"

	"a.yandex-team.ru/library/go/yandex/tvm"
)

const (
	DefaultProviderName    = "aws-tvm"
	DefaultSecretAccessKey = "unused"
)

var _ credentials.Provider = (*AuthProvider)(nil)

type AuthProvider struct {
	name            string
	accessKeyID     string
	secretAccessKey string
	tvmID           tvm.ClientID
	tvmClient       tvm.Client
}

func NewAuthProvider(tvmClient tvm.Client, opts ...Option) (*AuthProvider, error) {
	provider := &AuthProvider{
		name:            DefaultProviderName,
		secretAccessKey: DefaultSecretAccessKey,
		tvmClient:       tvmClient,
	}

	for _, opt := range opts {
		opt(provider)
	}

	if provider.tvmID == 0 {
		return nil, errors.New("destination service TVM ID is not provided")
	}

	return provider, nil
}

// Retrieve returns nil if it successfully retrieved the value.
// Error is returned if the value were not obtainable, or empty.
func (a *AuthProvider) Retrieve() (credentials.Value, error) {
	ticket, err := a.tvmClient.GetServiceTicketForID(context.Background(), a.tvmID)
	if err != nil {
		return credentials.Value{}, fmt.Errorf("get TVM ticket: %w", err)
	}

	return credentials.Value{
		AccessKeyID:     a.accessKeyID,
		SecretAccessKey: a.secretAccessKey,
		SessionToken:    ticket,
		ProviderName:    a.name,
	}, nil
}

// IsExpired returns if the credentials are no longer valid, and need
// to be retrieved.
func (a *AuthProvider) IsExpired() bool {
	return true
}
