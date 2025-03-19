package grpcclient

import (
	"context"
	"sync"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang/protobuf/ptypes"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"

	pb "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/dataproc/manager/v1"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/apiclient"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// CertConfig describes configuration for server TLS certificate
type CertConfig struct {
	File       string `json:"file" yaml:"file"`
	ServerName string `json:"server_name" yaml:"server_name"`
	Disabled   bool
}

// Config describes configuration for grpc client
type Config struct {
	GrpcTimeout time.Duration `json:"grpc_timeout" yaml:"grpc_timeout"`
	APIURL      string        `json:"dataproc_api_url" yaml:"dataproc_api_url"`
	Cert        CertConfig    `json:"cert" yaml:"cert"`
}

// DefaultConfig describes configuration for grpc client
func DefaultConfig() Config {
	return Config{
		GrpcTimeout: 5 * time.Second,
		APIURL:      "0.0.0.0:50051",
	}
}

type client struct {
	logger      log.Logger
	conn        *grpc.ClientConn
	connMutex   sync.Mutex
	cfg         Config
	getToken    models.GetToken
	getFolderID models.GetFolderID
	cid         string
	useragent   string
}

var _ apiclient.Client = &client{}

// New constructs grpc client
// It is a thread safe.
func New(
	cfg Config,
	getToken models.GetToken,
	getFolderID models.GetFolderID,
	cid,
	useragent string,
	logger log.Logger,
) (apiclient.Client, error) {
	client := &client{
		cfg:         cfg,
		getToken:    getToken,
		getFolderID: getFolderID,
		logger:      logger,
		cid:         cid,
		useragent:   useragent,
	}
	return client, nil
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
		c.cfg.APIURL,
		c.useragent,
		grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{
					CAFile:     c.cfg.Cert.File,
					ServerName: c.cfg.Cert.ServerName,
				},
			},
		},
		c.logger,
	)
	if err != nil {
		return nil, xerrors.Errorf("can not connect to URL %q: %w", c.cfg.APIURL, err)
	}
	c.conn = conn
	return conn, nil
}

func copyHbaseNodes(nodes []models.HbaseNodeInfo) []*pb.HbaseNodeInfo {
	copy := make([]*pb.HbaseNodeInfo, 0, len(nodes))
	for _, node := range nodes {
		nodeNew := &pb.HbaseNodeInfo{
			Name:          node.Name,
			Requests:      node.Requests,
			HeapSizeMb:    node.HeapSizeMB,
			MaxHeapSizeMb: node.MaxHeapSizeMB,
		}
		copy = append(copy, nodeNew)
	}
	return copy
}

func copyHdfsNodes(nodes []models.HDFSNodeInfo) []*pb.HDFSNodeInfo {
	copy := make([]*pb.HDFSNodeInfo, 0, len(nodes))
	for _, node := range nodes {
		nodeNew := &pb.HDFSNodeInfo{
			Name:      node.Name,
			Used:      node.Used,
			Remaining: node.Remaining,
			Capacity:  node.Capacity,
			NumBlocks: node.NumBlocks,
			State:     node.State,
		}
		copy = append(copy, nodeNew)
	}
	return copy
}

func copyYarnNodes(nodes []models.YARNNodeInfo) []*pb.YarnNodeInfo {
	copy := make([]*pb.YarnNodeInfo, 0, len(nodes))
	for _, node := range nodes {
		nodeNew := &pb.YarnNodeInfo{
			Name:              node.Name,
			State:             node.State,
			NumContainers:     node.NumContainers,
			UsedMemoryMb:      node.UsedMemoryMB,
			AvailableMemoryMb: node.AvailableMemoryMB,
			UpdateTime:        node.UpdateTime,
		}
		copy = append(copy, nodeNew)
	}
	return copy
}

func buildRequest(info models.Info) *pb.ReportRequest {
	return &pb.ReportRequest{
		Cid:              info.Cid,
		TopologyRevision: info.TopologyRevision,
		Info: &pb.Info{
			ReportCount: info.ReportCount,
			Hbase: &pb.HbaseInfo{
				Available:   info.HBase.Available,
				Regions:     info.HBase.Regions,
				Requests:    info.HBase.Requests,
				AverageLoad: info.HBase.AverageLoad,
				LiveNodes:   copyHbaseNodes(info.HBase.LiveNodes),
				DeadNodes:   copyHbaseNodes(info.HBase.DeadNodes),
			},
			Hdfs: &pb.HDFSInfo{
				Available:                  info.HDFS.Available,
				PercentRemaining:           info.HDFS.PercentRemaining,
				Used:                       info.HDFS.Used,
				Free:                       info.HDFS.Free,
				TotalBlocks:                info.HDFS.TotalBlocks,
				MissingBlocks:              info.HDFS.NumberOfMissingBlocks,
				MissingBlocksReplicaOne:    info.HDFS.NumberOfMissingBlocksWithReplicationFactorOne,
				LiveNodes:                  copyHdfsNodes(info.HDFS.LiveNodes),
				DeadNodes:                  copyHdfsNodes(info.HDFS.DeadNodes),
				DecommissioningNodes:       copyHdfsNodes(info.HDFS.DecommissioningNodes),
				DecommissionedNodes:        copyHdfsNodes(info.HDFS.DecommissionedNodes),
				Safemode:                   info.HDFS.Safemode,
				RequestedDecommissionHosts: info.HDFS.RequestedDecommissionHosts,
			},
			Hive: &pb.HiveInfo{
				Available:        info.Hive.Available,
				QueriesSucceeded: info.Hive.QueriesSucceeded,
				QueriesExecuting: info.Hive.QueriesExecuting,
				QueriesFailed:    info.Hive.QueriesFailed,
				SessionsOpen:     info.Hive.SessionsOpen,
				SessionsActive:   info.Hive.SessionsActive,
			},
			Yarn: &pb.YarnInfo{
				Available:                  info.YARN.Available,
				LiveNodes:                  copyYarnNodes(info.YARN.LiveNodes),
				RequestedDecommissionHosts: info.YARN.RequestedDecommissionHosts,
			},
			Zookeeper: &pb.ZookeeperInfo{
				Alive: info.Zookeeper.Available,
			},
			Oozie: &pb.OozieInfo{
				Alive: info.Oozie.Available,
			},
			Livy: &pb.LivyInfo{
				Alive: info.Livy.Available,
			},
		},
		CollectedAt: ptypes.TimestampNow(),
	}
}

// Report sends health report via grpc
func (c *client) Report(ctx context.Context, info models.Info) (*models.NodesToDecommission, error) {
	var responseHeader metadata.MD

	conn, err := c.getConnection(ctx)
	if err != nil {
		return nil, err
	}
	grpcClient := pb.NewDataprocManagerServiceClient(conn)
	ctx, cancel := context.WithTimeout(ctx, c.cfg.GrpcTimeout)
	defer cancel()

	var reportReply *pb.ReportReply
	reportReply, err = grpcClient.Report(c.withMeta(ctx), buildRequest(info), grpc.Header(&responseHeader))
	c.logger.Debugf("Reply headers: %+v", responseHeader)
	if err != nil {
		return nil, xerrors.Errorf("can not call method 'Report': %w", err)
	}
	nodesToDecommission := &models.NodesToDecommission{
		DecommissionTimeout:     int(reportReply.DecommissionTimeout),
		YarnHostsToDecommission: reportReply.YarnHostsToDecommission,
		HdfsHostsToDecommission: reportReply.HdfsHostsToDecommission,
	}
	return nodesToDecommission, nil
}

func (c *client) withMeta(ctx context.Context) context.Context {
	meta := metadata.Pairs(
		"authorization", c.getToken(),
		"authorization", c.getFolderID(),
	)

	requestID, err := uuid.NewV4()
	if err != nil {
		c.logger.Errorf("Failed to generate requestID UUID: %s", err)
		return metadata.NewOutgoingContext(ctx, meta)
	}
	traceID, err := uuid.NewV4()
	if err != nil {
		c.logger.Errorf("Failed to generate traceID UUID: %s", err)
		return metadata.NewOutgoingContext(ctx, meta)
	}
	c.logger.Debugf("Request ID: %s, Trace ID: %s", requestID, traceID)

	meta.Set("x-client-request-id", requestID.String())
	meta.Set("x-client-trace-id", traceID.String())

	return metadata.NewOutgoingContext(ctx, meta)
}
