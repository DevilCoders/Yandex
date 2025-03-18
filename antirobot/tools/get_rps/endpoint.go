package main

import (
	"a.yandex-team.ru/infra/yp_service_discovery/golang/resolver"
	"a.yandex-team.ru/infra/yp_service_discovery/golang/resolver/cachedresolver"
	"context"
)

type EndpointConfig struct {
	DC        string   `yaml:"dc"`
	Endpoints []string `yaml:"endpoint"`
}

func ResolveEndpoints(ctx context.Context, cr *cachedresolver.CachedResolver, cluster string, endpointSet string) ([]*resolver.Endpoint, error) {
	endpoints := make([]*resolver.Endpoint, 0)

	resp, err := cr.ResolveEndpoints(ctx, cluster, endpointSet)
	if err != nil {
		return nil, err
	}
	if resp.ResolveStatus != resolver.StatusEndpointNotExists &&
		resp.EndpointSet != nil {
		endpoints = resp.EndpointSet.Endpoints
	}
	return endpoints, nil
}

func ResolveAllEndpoints(ctx context.Context, cr *cachedresolver.CachedResolver, config []EndpointConfig) ([]*resolver.Endpoint, error) {
	var result []*resolver.Endpoint
	for _, c := range config {
		cluster := c.DC
		for _, endpointSet := range c.Endpoints {
			endpoints, err := ResolveEndpoints(ctx, cr, cluster, endpointSet)
			if err != nil {
				return nil, err
			}
			result = append(result, endpoints...)
		}
	}
	return result, nil
}
