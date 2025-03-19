package kmslbclient

import (
	"context"
	"crypto/tls"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1"
)

type KeyValidator interface {
	// Checks if key is valid.
	ValidateKey(ctx context.Context, keyID string, callCreds *CallCredentials) (bool, error)
}

type keyValidator struct {
	cc *grpc.ClientConn
}

func (v *keyValidator) ValidateKey(ctx context.Context, keyID string, callCreds *CallCredentials) (bool, error) {
	sk := kms.NewSymmetricKeyServiceClient(v.cc)
	ctx, err := appendAuthorization(ctx, nil, callCreds)
	if err != nil {
		return false, err
	}
	_, err = sk.Get(ctx, &kms.GetSymmetricKeyRequest{KeyId: keyID})
	code := status.Code(err)
	switch code {
	case codes.OK:
		return true, nil
	case codes.NotFound, codes.PermissionDenied, codes.FailedPrecondition, codes.InvalidArgument:
		return false, nil
	default:
		return false, err
	}
}

func NewKeyValidator(targetAddr string) (KeyValidator, error) {
	transportCredsOption := grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{}))
	keepaliveOption := grpc.WithKeepaliveParams(keepalive.ClientParameters{
		// Use HTTP2 pings with half keepalive time so that connections are guaranteed checked between
		// two keep-alive checks and rebalances.
		// NOTE: We could make this configurable, but this would make the interface more complicated
		// with little to no actual usecases.
		Time:                defaultOptions.Balancer.KeepAliveTime,
		Timeout:             defaultOptions.Balancer.KeepAliveTime,
		PermitWithoutStream: false,
	})
	cc, err := grpc.Dial(targetAddr, transportCredsOption, keepaliveOption)
	if err != nil {
		return nil, err
	}
	return &keyValidator{cc: cc}, nil
}
