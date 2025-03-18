package admin

import (
	"context"

	"a.yandex-team.ru/library/go/yandex/solomon"
)

func (c *Client) ListClusters(ctx context.Context, opts ...solomon.ListOption) ([]solomon.Cluster, error) {
	var clusters []solomon.Cluster
	err := c.ListObjects(ctx, solomon.TypeCluster, &clusters, opts...)
	return clusters, err
}

func (c *Client) FindCluster(ctx context.Context, id string) (solomon.Cluster, error) {
	var cluster solomon.Cluster
	err := c.FindObject(ctx, solomon.TypeCluster, id, &cluster)
	return cluster, err
}

func (c *Client) CreateCluster(ctx context.Context, cluster solomon.Cluster) (solomon.Cluster, error) {
	err := c.CreateObject(ctx, solomon.TypeCluster, &cluster)
	return cluster, err
}

func (c *Client) UpdateCluster(ctx context.Context, cluster solomon.Cluster) (solomon.Cluster, error) {
	err := c.UpdateObject(ctx, solomon.TypeCluster, cluster.ID, &cluster)
	return cluster, err
}

func (c *Client) DeleteCluster(ctx context.Context, id string) error {
	return c.DeleteObject(ctx, solomon.TypeCluster, id)
}
