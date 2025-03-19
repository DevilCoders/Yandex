package kmslbclient

import (
	"context"
	"crypto/tls"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"

	kms_discovery "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1/discovery"
	"a.yandex-team.ru/library/go/core/log"
)

type discoveryResolver struct {
	service string
	logger  log.Logger

	discoveryConn *grpc.ClientConn
	// Cache last discovered addresses in case the discovery call fails.
	lastAddrs []string
}

func (r *discoveryResolver) GetEndpoints(ctx context.Context) ([]ResolvedAddr, error) {
	client := kms_discovery.NewDiscoveryServiceClient(r.discoveryConn)
	ret, err := client.GetEndpoints(ctx, &kms_discovery.GetEndpointsRequest{
		Service: r.service,
	})
	if err != nil || len(ret.Endpoints) == 0 {
		r.logger.Errorf("Got error %v, endpoints %v during discovery", err, ret)
		if len(r.lastAddrs) > 0 {
			return toResolvedAddr(r.lastAddrs), nil
		} else if err != nil {
			return nil, err
		} else {
			return []ResolvedAddr{}, nil
		}
	}
	r.lastAddrs = fromEndpoints(ret.Endpoints)
	return toResolvedAddr(r.lastAddrs), nil
}

func toResolvedAddr(addrs []string) []ResolvedAddr {
	ret := make([]ResolvedAddr, len(addrs))
	for i := 0; i < len(addrs); i++ {
		ret[i].addr = addrs[i]
		ret[i].name = addrs[i]
	}
	return ret
}

func fromEndpoints(endpoints []*kms_discovery.EndpointInfo) []string {
	ret := make([]string, len(endpoints))
	for i := 0; i < len(endpoints); i++ {
		ret[i] = endpoints[i].Address
	}
	return ret
}

func (r *discoveryResolver) Close() {
	_ = r.discoveryConn.Close()
}

func NewDiscoveryResolver(discoveryAddr string, service string, options *ClientOptions) (Resolver, error) {
	var dialOptions []grpc.DialOption
	if options.Plaintext {
		dialOptions = append(dialOptions, grpc.WithInsecure())
	} else {
		dialOptions = append(dialOptions,
			grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{
				ServerName: options.TLSTarget,
			})))
	}
	dialOptions = append(dialOptions, grpc.WithKeepaliveParams(keepalive.ClientParameters{
		Time:                options.Balancer.KeepAliveTime,
		Timeout:             time.Second,
		PermitWithoutStream: false,
	}))
	conn, err := grpc.Dial(discoveryAddr, dialOptions...)
	if err != nil {
		return nil, err
	}
	ret := &discoveryResolver{
		service:       service,
		logger:        options.Logger,
		discoveryConn: conn,
	}
	return ret, nil
}
