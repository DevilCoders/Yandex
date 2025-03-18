package solomon

import "context"

type (
	ObjectType string
)

const (
	TypeService   ObjectType = "services"
	TypeCluster   ObjectType = "clusters"
	TypeShard     ObjectType = "shards"
	TypeAlert     ObjectType = "alerts"
	TypeDashboard ObjectType = "dashboards"
)

type AdminClient interface {
	// Generic interface to manipulating any kind of object.
	ListObjects(ctx context.Context, typ ObjectType, slice interface{}, opts ...ListOption) error
	// FindObject looks up object by id, filling o with resulting object value.
	FindObject(ctx context.Context, typ ObjectType, id string, o interface{}) error
	// CreateObject creates new object, filling o with resulting object value.
	CreateObject(ctx context.Context, typ ObjectType, o interface{}) error
	// UpdateObject updates existing object, filling o with resulting object value.
	UpdateObject(ctx context.Context, typ ObjectType, id string, o interface{}) error
	// DeleteObject removes existing object.
	DeleteObject(ctx context.Context, typ ObjectType, id string) error

	// Generic methods for manipulating services.
	ListServices(ctx context.Context, opts ...ListOption) ([]Service, error)
	FindService(ctx context.Context, id string) (Service, error)
	CreateService(ctx context.Context, service Service) (Service, error)
	UpdateService(ctx context.Context, service Service) (Service, error)
	DeleteService(ctx context.Context, id string) error

	// Generic methods for manipulating clusters.
	ListClusters(ctx context.Context, opts ...ListOption) ([]Cluster, error)
	FindCluster(ctx context.Context, id string) (Cluster, error)
	CreateCluster(ctx context.Context, cluster Cluster) (Cluster, error)
	UpdateCluster(ctx context.Context, cluster Cluster) (Cluster, error)
	DeleteCluster(ctx context.Context, id string) error

	// Generic methods for manipulating shards.
	ListShards(ctx context.Context, opts ...ListOption) ([]Shard, error)
	FindShard(ctx context.Context, id string) (Shard, error)
	CreateShard(ctx context.Context, shard Shard) (Shard, error)
	UpdateShard(ctx context.Context, shard Shard) (Shard, error)
	DeleteShard(ctx context.Context, id string) error

	// Specific shard methods.
	ListServiceShards(ctx context.Context, serviceID string) ([]ClusterService, error)
	ListClusterShards(ctx context.Context, clusterID string) ([]ClusterService, error)
	ListTargetStatus(ctx context.Context, shardID string) ([]HostStatus, error)

	// Generic methods for manipulating alerts.
	ListAlerts(ctx context.Context, opts ...ListOption) ([]AlertDescription, error)
	FindAlert(ctx context.Context, id string) (Alert, error)
	CreateAlert(ctx context.Context, alert Alert) (Alert, error)
	UpdateAlert(ctx context.Context, alert Alert) (Alert, error)
	DeleteAlert(ctx context.Context, id string) error
}

func ListAllAlerts(ctx context.Context, c AdminClient, filter AlertFilter) ([]AlertDescription, error) {
	var alerts []AlertDescription
	var nextToken NextPageToken

	for {
		var pageAlerts []AlertDescription
		var err error

		if nextToken.NextToken != "" {
			pageAlerts, err = c.ListAlerts(ctx, filter, &nextToken, PageToken{Token: nextToken.NextToken})
		} else {
			pageAlerts, err = c.ListAlerts(ctx, filter, &nextToken)
		}

		if err != nil {
			return nil, err
		}

		alerts = append(alerts, pageAlerts...)
		if nextToken.NextToken == "" {
			break
		}
	}

	return alerts, nil
}
