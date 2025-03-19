package yc

import (
	"context"
	"net/url"
)

// implementation of:
//
// type PerRPCCredentials interface {
// 	 GetRequestMetadata(ctx context.Context, uri ...string) (map[string]string, error)
// 	 RequireTransportSecurity() bool
// }

type rpcCreds struct {
	token string
}

const iamTokenServicePath = "/yandex.cloud.iam.v1.IamTokenService"
const APIEndpointServicePath = "/yandex.cloud.endpoint.ApiEndpointService"

func (c *rpcCreds) GetRequestMetadata(ctx context.Context, uri ...string) (map[string]string, error) {
	audienceURL, err := url.Parse(uri[0])
	if err != nil {
		return nil, err
	}
	if audienceURL.Path == iamTokenServicePath || audienceURL.Path == APIEndpointServicePath {
		return nil, nil
	}

	return map[string]string{
		"authorization": "Bearer " + c.token,
	}, nil
}

func (c *rpcCreds) RequireTransportSecurity() bool {
	return true
}
