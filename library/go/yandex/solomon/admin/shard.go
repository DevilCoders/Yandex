package admin

import (
	"context"
	"fmt"

	"a.yandex-team.ru/library/go/yandex/solomon"
)

func (c *Client) ListShards(ctx context.Context, opts ...solomon.ListOption) ([]solomon.Shard, error) {
	var shards []solomon.Shard
	err := c.ListObjects(ctx, solomon.TypeShard, &shards, opts...)
	return shards, err
}

func (c *Client) listShards(ctx context.Context, typ solomon.ObjectType, id string) ([]solomon.ClusterService, error) {
	var apiErr solomon.APIError
	var services []solomon.ClusterService

	var path string
	switch typ {
	case solomon.TypeCluster:
		path = fmt.Sprintf("/clusters/%s/services", id)
	case solomon.TypeService:
		path = fmt.Sprintf("/services/%s/clusters", id)
	default:
		return nil, fmt.Errorf("type %q has no associated shards", typ)
	}

	rsp, err := c.r(ctx).
		SetError(&apiErr).
		SetResult(&services).
		SetQueryParam("pageSize", "all").
		Get(path)

	if err != nil {
		return nil, err
	}

	if rsp.IsError() {
		return nil, &apiErr
	}

	return services, nil

}

func (c *Client) ListServiceShards(ctx context.Context, serviceID string) ([]solomon.ClusterService, error) {
	return c.listShards(ctx, solomon.TypeService, serviceID)
}

func (c *Client) ListClusterShards(ctx context.Context, clusterID string) ([]solomon.ClusterService, error) {
	return c.listShards(ctx, solomon.TypeCluster, clusterID)
}

func (c *Client) FindShard(ctx context.Context, id string) (solomon.Shard, error) {
	var shard solomon.Shard
	err := c.FindObject(ctx, solomon.TypeShard, id, &shard)
	return shard, err
}

func (c *Client) CreateShard(ctx context.Context, shard solomon.Shard) (solomon.Shard, error) {
	err := c.CreateObject(ctx, solomon.TypeShard, &shard)
	return shard, err
}

func (c *Client) UpdateShard(ctx context.Context, shard solomon.Shard) (solomon.Shard, error) {
	err := c.UpdateObject(ctx, solomon.TypeShard, shard.ID, &shard)
	return shard, err
}

func (c *Client) DeleteShard(ctx context.Context, id string) error {
	return c.DeleteObject(ctx, solomon.TypeShard, id)
}

func (c *Client) ListTargetStatus(ctx context.Context, shardID string) ([]solomon.HostStatus, error) {
	var apiErr solomon.APIError
	var result struct {
		Result []solomon.HostStatus `json:"result"`
	}

	rsp, err := c.r(ctx).
		SetError(&apiErr).
		SetResult(&result).
		SetQueryParam("pageSize", "all").
		SetPathParams(map[string]string{"shardID": shardID}).
		Get("/shards/{shardID}/targets")

	if err != nil {
		return nil, err
	}

	if rsp.IsError() {
		return nil, &apiErr
	}

	return result.Result, nil
}
