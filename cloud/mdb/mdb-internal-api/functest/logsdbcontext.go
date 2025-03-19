package functest

import (
	"strings"

	"github.com/golang/protobuf/proto"
	"github.com/jhump/protoreflect/dynamic"
	"gopkg.in/yaml.v2"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	mongov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mongodb/v1"
	mysqlv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mysql/v1"
	osv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/opensearch/v1"
	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	redisv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/redis/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	grpcch "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/clickhouse"
	grpces "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/elasticsearch"
	grpcgp "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/greenplum"
	grpcmongo "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/mongodb"
	grpcmysql "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/mysql"
	grpcos "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/opensearch"
	grpcpg "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/postgresql"
	grpcredis "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/redis"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	logsdbch "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/clickhouse/sqlmock"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type LogsDBContext struct {
	l    log.Logger
	Mock *sqlmock.Mock
}

func NewLogsDBContext(l log.Logger) (*LogsDBContext, error) {
	mock, err := sqlmock.New()
	if err != nil {
		return nil, err
	}

	return &LogsDBContext{l: l, Mock: mock}, nil
}

func (lc *LogsDBContext) Reset() {
	lc.Mock.Reset()
}

var requestConverters = []func(*dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error){
	func(dyn *dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
		req := &chv1.ListClusterLogsRequest{}
		if err := dyn.ConvertTo(req); err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		lst, err := grpcch.ListLogsServiceTypeFromGRPC(req.GetServiceType())
		if err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		return req, lst, nil
	},
	func(dyn *dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
		req := &mongov1.ListClusterLogsRequest{}
		if err := dyn.ConvertTo(req); err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		lst, err := grpcmongo.ListLogsServiceTypeFromGRPC(req.GetServiceType())
		if err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		return req, lst, nil
	},
	func(dyn *dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
		req := &mysqlv1.ListClusterLogsRequest{}
		if err := dyn.ConvertTo(req); err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		lst, err := grpcmysql.ListLogsServiceTypeFromGRPC(req.GetServiceType())
		if err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		return req, lst, nil
	},
	func(dyn *dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
		req := &pgv1.ListClusterLogsRequest{}
		if err := dyn.ConvertTo(req); err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		lst, err := grpcpg.ListLogsServiceTypeFromGRPC(req.GetServiceType())
		if err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		return req, lst, nil
	},
	func(dyn *dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
		req := &redisv1.ListClusterLogsRequest{}
		if err := dyn.ConvertTo(req); err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		lst, err := grpcredis.ListLogsServiceTypeFromGRPC(req.GetServiceType())
		if err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		return req, lst, nil
	},
	func(dyn *dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
		req := &esv1.ListClusterLogsRequest{}
		if err := dyn.ConvertTo(req); err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		lst, err := grpces.ListLogsServiceTypeFromGRPC(req.GetServiceType())
		if err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		return req, lst, nil
	},
	func(dyn *dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
		req := &kfv1.ListClusterLogsRequest{}
		if err := dyn.ConvertTo(req); err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		return req, logs.ServiceTypeKafka, nil
	},
	func(dyn *dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
		req := &gpv1.ListClusterLogsRequest{}
		if err := dyn.ConvertTo(req); err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}
		lst, err := grpcgp.ListLogsServiceTypeFromGRPC(req.GetServiceType())
		if err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		return req, lst, nil
	},
	func(dyn *dynamic.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
		req := &osv1.ListClusterLogsRequest{}
		if err := dyn.ConvertTo(req); err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}
		lst, err := grpcos.ListLogsServiceTypeFromGRPC(req.GetServiceType())
		if err != nil {
			return nil, logs.ServiceTypeInvalid, err
		}

		return req, lst, nil
	},
}

func convertGRPCLogsRequest(msg proto.Message) (grpc.ListClusterLogsRequest, logs.ServiceType, error) {
	dyn := msg.(*dynamic.Message)
	for _, f := range requestConverters {
		req, st, err := f(dyn)
		if err != nil {
			continue
		}

		return req, st, nil
	}

	return nil, logs.ServiceTypeInvalid, xerrors.Errorf("couldn't convert gRPC message %q: no known converter found", msg.String())
}

func (lc *LogsDBContext) MaybeExpect(method, service string, msg proto.Message) error {
	if !strings.HasSuffix(service, "ClusterService") {
		return nil
	}

	if method != "ListLogs" {
		return nil
	}

	// Extract data from proto message
	req, st, err := convertGRPCLogsRequest(msg)
	if err != nil {
		return xerrors.Errorf("failed to convert generalized gRPC logs request: %w", err)
	}

	// Set correct page size (with default)
	pageSize := req.GetPageSize()
	if pageSize == 0 {
		pageSize = 100
	}

	// Parse page token
	pageToken, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		lc.l.Warnf("fail to parse pageToken, not setting any expectations: %s", err)
		return nil
	}

	flt, err := grpc.FilterFromGRPC(req.GetFilter())
	if err != nil {
		lc.l.Warnf("fail to parse filter, not setting any expectations: %s", err)
		return nil
	}

	// Get expected query and params
	// hasRows emulates double-request for pooler service type
	var hasRows bool
	for _, lst := range logsdb.LogsServiceTypeToLogType(st) {
		if hasRows {
			return nil
		}

		// Ignore error - we might be checking invalid params
		if err := lc.Mock.SetExpectations(req.GetClusterId(), lst, req.GetColumnFilter(), flt, pageSize, pageToken); err != nil {
			lc.l.Warnf("failed to set expectations for logsdb, ignoring: %s", err)
		}
	}

	return nil
}

func (lc *LogsDBContext) AssertExpectations() error {
	return lc.Mock.AssertExpectations()
}

func (lc *LogsDBContext) FillData(data []byte, named bool) error {
	var parsed [][]string
	if err := yaml.Unmarshal(data, &parsed); err != nil {
		return xerrors.Errorf("failed to unmarshal logsdb data %s: %w", string(data), err)
	}

	if len(parsed) == 0 {
		return nil
	}

	count := len(parsed[0])
	names := []string{logsdbch.ParamSeconds, logsdbch.ParamMilliseconds, logsdbch.ParamMessage}
	var converted []map[string]string

	for i, values := range parsed {
		// Set column names if needed
		if named && i == 0 {
			names = append([]string{}, values...)
			continue
		}

		// Check number of columns
		if len(values) != count {
			return xerrors.Errorf("expected row length of %d but has %d", count, len(values))
		}

		// Construct row
		row := make(map[string]string, len(values))
		for i, value := range values {
			row[names[i]] = value
		}

		converted = append(converted, row)
	}

	return lc.Mock.FillData(converted)
}
