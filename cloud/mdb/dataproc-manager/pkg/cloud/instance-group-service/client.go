package cloud

import (
	"context"
	"strings"
	"sync"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"

	ig "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/microcosm/instancegroup/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	iam "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/cloud/iam-token-service"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type InstanceGroupServiceConfig struct {
	CAPath      string        `json:"capath" yaml:"capath"`
	ServiceURL  string        `json:"service_url" yaml:"service_url"`
	GrpcTimeout time.Duration `json:"grpc_timeout" yaml:"grpc_timeout"`
}

// DefaultConfig describes configuration for grpc client
func DefaultConfig() InstanceGroupServiceConfig {
	return InstanceGroupServiceConfig{
		CAPath:      "/opt/yandex/allCAs.pem",
		ServiceURL:  "instance-group.private-api.ycp.cloud-preprod.yandex.net:443",
		GrpcTimeout: 30 * time.Second,
	}
}

type client struct {
	logger                log.Logger
	conn                  *grpc.ClientConn
	connMutex             sync.Mutex
	config                InstanceGroupServiceConfig
	useragent             string
	iamTokenServiceClient iam.IamTokenServiceClient
}

type InstanceGroupServiceClient interface {
	Get(ctx context.Context, instanceGroupID string, serviceAccountID string) (*ig.InstanceGroup, error)
	DeleteInstances(ctx context.Context, instanceGroupID string, managedInstanceIDs []string, serviceAccountID string, subclusterID string) (*operation.Operation, error)
	StopInstances(ctx context.Context, instanceGroupID string, managedInstanceIDs []string, serviceAccountID string, subclusterID string) (*operation.Operation, error)
	ListInstances(ctx context.Context, instanceGroupID string, serviceAccountID string) ([]*ig.ManagedInstance, error)
	IsRunning(instance *ig.ManagedInstance) bool
	IsPreferredForDecommission(instance *ig.ManagedInstance) bool
	GetUserSpecifiedFQDN(instance *ig.ManagedInstance) string
}

var _ InstanceGroupServiceClient = &client{}

// New constructs grpc client
// It is a thread safe.
func New(
	config InstanceGroupServiceConfig,
	logger log.Logger,
	iamTokenServiceClient iam.IamTokenServiceClient) InstanceGroupServiceClient {
	client := &client{
		config:                config,
		logger:                logger,
		useragent:             "Dataproc-Manager",
		iamTokenServiceClient: iamTokenServiceClient,
	}
	return client
}

func (c *client) getConnection(ctx context.Context) (*grpc.ClientConn, error) {
	if c.conn != nil {
		return c.conn, nil
	}

	c.connMutex.Lock()
	defer c.connMutex.Unlock()

	if c.conn != nil {
		return c.conn, nil
	}

	conn, err := grpcutil.NewConn(
		ctx,
		c.config.ServiceURL,
		c.useragent,
		grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{
					CAFile: c.config.CAPath,
				},
			},
		},
		c.logger,
	)
	if err != nil {
		return nil, xerrors.Errorf("can not connect to URL %q: %w", c.config.ServiceURL, err)
	}
	c.conn = conn
	return conn, nil
}

func (c *client) Get(ctx context.Context, instanceGroupID string, serviceAccountID string) (*ig.InstanceGroup, error) {
	iamToken, err := c.iamTokenServiceClient.CreateForServiceAccount(ctx, serviceAccountID)
	if err != nil {
		return nil, xerrors.Errorf("can not get IAM token for service account %s: %w", serviceAccountID, err)
	}
	conn, err := c.getConnection(ctx)
	if err != nil {
		return nil, err
	}
	grpcClient := ig.NewInstanceGroupServiceClient(conn)
	ctx, cancel := context.WithTimeout(ctx, c.config.GrpcTimeout)
	defer cancel()

	var instanceGroup *ig.InstanceGroup
	instanceGroup, err = grpcClient.Get(c.withHeaders(ctx, iamToken, ""), &ig.GetInstanceGroupRequest{InstanceGroupId: instanceGroupID})
	if err != nil {
		return nil, xerrors.Errorf("can not call method 'GetInstanceGroup': %w", err)
	}
	return instanceGroup, nil
}

func (c *client) DeleteInstances(ctx context.Context, instanceGroupID string, managedInstanceIDs []string, serviceAccountID string, subclusterID string) (*operation.Operation, error) {
	iamToken, err := c.iamTokenServiceClient.CreateForServiceAccount(ctx, serviceAccountID)
	if err != nil {
		return nil, xerrors.Errorf("can not get IAM token for service account %s: %w", serviceAccountID, err)
	}
	conn, err := c.getConnection(ctx)
	if err != nil {
		return nil, err
	}
	grpcClient := ig.NewInstanceGroupServiceClient(conn)
	ctx, cancel := context.WithTimeout(ctx, c.config.GrpcTimeout)
	defer cancel()

	var op *operation.Operation
	op, err = grpcClient.DeleteInstances(
		c.withHeaders(ctx, iamToken, subclusterID),
		&ig.DeleteInstancesRequest{InstanceGroupId: instanceGroupID, ManagedInstanceIds: managedInstanceIDs},
	)
	if err != nil {
		return nil, xerrors.Errorf("can not call method 'DeleteInstance': %w", err)
	}
	return op, nil
}

func (c *client) StopInstances(ctx context.Context, instanceGroupID string, managedInstanceIDs []string, serviceAccountID string, subclusterID string) (*operation.Operation, error) {
	iamToken, err := c.iamTokenServiceClient.CreateForServiceAccount(ctx, serviceAccountID)
	if err != nil {
		return nil, xerrors.Errorf("can not get IAM token for service account %s: %w", serviceAccountID, err)
	}
	conn, err := c.getConnection(ctx)
	if err != nil {
		return nil, err
	}
	grpcClient := ig.NewInstanceGroupServiceClient(conn)
	ctx, cancel := context.WithTimeout(ctx, c.config.GrpcTimeout)
	defer cancel()

	var op *operation.Operation
	op, err = grpcClient.StopInstances(
		c.withHeaders(ctx, iamToken, subclusterID),
		&ig.StopInstancesRequest{InstanceGroupId: instanceGroupID, ManagedInstanceIds: managedInstanceIDs},
	)
	if err != nil {
		return nil, xerrors.Errorf("can not call method 'DeleteInstance': %w", err)
	}
	return op, nil
}

func (c *client) ListInstances(ctx context.Context, instanceGroupID string, serviceAccountID string) ([]*ig.ManagedInstance, error) {
	iamToken, err := c.iamTokenServiceClient.CreateForServiceAccount(ctx, serviceAccountID)
	if err != nil {
		return nil, xerrors.Errorf("can not get IAM token for service account %s: %w", serviceAccountID, err)
	}
	conn, err := c.getConnection(ctx)
	if err != nil {
		return nil, err
	}
	grpcClient := ig.NewInstanceGroupServiceClient(conn)
	ctx, cancel := context.WithTimeout(ctx, c.config.GrpcTimeout)
	defer cancel()

	response, err := grpcClient.ListInstances(
		c.withHeaders(ctx, iamToken, ""),
		&ig.ListInstanceGroupInstancesRequest{InstanceGroupId: instanceGroupID},
	)

	if err != nil {
		return nil, xerrors.Errorf("can not call method 'ListInstances': %w", err)
	}
	return response.Instances, nil
}

func (c *client) IsRunning(instance *ig.ManagedInstance) bool {
	return instance.GetStatus() == ig.ManagedInstance_RUNNING_ACTUAL
}

func (c *client) GetUserSpecifiedFQDN(instance *ig.ManagedInstance) string {
	// GET MDB-formatted additional FQDN like "rc1b-dataproc-g-ylyn.mdb.cloud-preprod.yandex.net"
	// instead of internal fqdn like "rc1b-dataproc-g-ylyn.ru-central1.internal"
	for _, network := range instance.NetworkInterfaces {
		if network.GetPrimaryV4Address() != nil && network.GetPrimaryV4Address().GetDnsRecords() != nil {
			for _, dnsRecord := range network.GetPrimaryV4Address().GetDnsRecords() {
				fqdn := dnsRecord.GetFqdn()
				if fqdn != "" {
					return strings.TrimSuffix(fqdn, ".")
				}
			}
		}
		if network.GetPrimaryV6Address() != nil && network.GetPrimaryV6Address().GetDnsRecords() != nil {
			for _, dnsRecord := range network.GetPrimaryV6Address().GetDnsRecords() {
				fqdn := dnsRecord.GetFqdn()
				if fqdn != "" {
					return strings.TrimSuffix(fqdn, ".")
				}
			}
		}
	}
	// unexpected backoff
	c.logger.Errorf("could not find user specified FQDN for instance %s", instance.GetInstanceId())
	return instance.GetFqdn()
}

func (c *client) IsPreferredForDecommission(instance *ig.ManagedInstance) bool {
	status := instance.GetStatus()
	isNotRunning := status != ig.ManagedInstance_RUNNING_ACTUAL
	isNotDeleting := status != ig.ManagedInstance_DELETING_INSTANCE
	isNotDeleted := status != ig.ManagedInstance_DELETED
	return isNotRunning && isNotDeleting && isNotDeleted
}

func (c *client) withHeaders(ctx context.Context, iamToken string, referrerID string) context.Context {
	md, ok := metadata.FromOutgoingContext(ctx)
	if !ok {
		md = metadata.MD{}
	} else {
		md = md.Copy()
	}

	md.Set("Authorization", "Bearer "+iamToken)
	md.Set("Referrer-ID", referrerID)
	return metadata.NewOutgoingContext(ctx, md)
}
