package kmslbclient

import "context"

type passthroughResolver struct {
	endpoints []ResolvedAddr
}

func (r *passthroughResolver) GetEndpoints(ctx context.Context) ([]ResolvedAddr, error) {
	return r.endpoints, nil
}

func (r *passthroughResolver) Close() {
}

type ResolvedAddr struct {
	name string
	addr string
}

type Resolver interface {
	GetEndpoints(ctx context.Context) ([]ResolvedAddr, error)
	Close()
}

func NewPassthroughResolver(targets []string) Resolver {
	var endpoints []ResolvedAddr
	for _, target := range targets {
		endpoints = append(endpoints, ResolvedAddr{
			name: target,
			addr: target,
		})
	}
	return &passthroughResolver{endpoints: endpoints}
}
