package auth

import (
	"context"
)

type GRPCTokenAuthenticator struct {
	TokenGetter
}

func (t *GRPCTokenAuthenticator) GetRequestMetadata(ctx context.Context, uri ...string) (map[string]string, error) {
	token, err := t.GetToken()
	if err != nil {
		return nil, err
	}
	return map[string]string{
		"authorization": "Bearer " + token,
	}, nil
}

func (*GRPCTokenAuthenticator) RequireTransportSecurity() bool {
	return true
}
