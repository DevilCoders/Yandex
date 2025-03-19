package mongodb

import (
	mongov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mongodb/v1"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

var (
	mapListLogsServiceTypeToGRPC = map[logs.ServiceType]mongov1.ListClusterLogsRequest_ServiceType{
		logs.ServiceTypeMongoD:       mongov1.ListClusterLogsRequest_MONGOD,
		logs.ServiceTypeMongoS:       mongov1.ListClusterLogsRequest_MONGOS,
		logs.ServiceTypeMongoCFG:     mongov1.ListClusterLogsRequest_MONGOCFG,
		logs.ServiceTypeMongoDBAudit: mongov1.ListClusterLogsRequest_AUDIT,
	}
	mapListLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapListLogsServiceTypeToGRPC).(map[mongov1.ListClusterLogsRequest_ServiceType]logs.ServiceType)
)

func ListLogsServiceTypeToGRPC(st logs.ServiceType) mongov1.ListClusterLogsRequest_ServiceType {
	v, ok := mapListLogsServiceTypeToGRPC[st]
	if !ok {
		return mongov1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func ListLogsServiceTypeFromGRPC(st mongov1.ListClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapListLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

var (
	mapStreamLogsServiceTypeToGRPC = map[logs.ServiceType]mongov1.StreamClusterLogsRequest_ServiceType{
		logs.ServiceTypeMongoD:       mongov1.StreamClusterLogsRequest_MONGOD,
		logs.ServiceTypeMongoS:       mongov1.StreamClusterLogsRequest_MONGOS,
		logs.ServiceTypeMongoCFG:     mongov1.StreamClusterLogsRequest_MONGOCFG,
		logs.ServiceTypeMongoDBAudit: mongov1.StreamClusterLogsRequest_AUDIT,
	}
	mapStreamLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapStreamLogsServiceTypeToGRPC).(map[mongov1.StreamClusterLogsRequest_ServiceType]logs.ServiceType)
)

func StreamLogsServiceTypeToGRPC(st logs.ServiceType) mongov1.StreamClusterLogsRequest_ServiceType {
	v, ok := mapStreamLogsServiceTypeToGRPC[st]
	if !ok {
		return mongov1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func StreamLogsServiceTypeFromGRPC(st mongov1.StreamClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapStreamLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

func ClusterToGRPC(cluster clusters.ClusterExtended) *mongov1.Cluster {
	v := &mongov1.Cluster{
		Id:                 cluster.ClusterID,
		FolderId:           cluster.FolderExtID,
		Name:               cluster.Name,
		Description:        cluster.Description,
		Labels:             cluster.Labels,
		NetworkId:          cluster.NetworkID,
		SecurityGroupIds:   cluster.SecurityGroupIDs,
		DeletionProtection: cluster.DeletionProtection,
	}
	// TODO: set CreatedAt, Environment, Monitoring, Config, Health and Status
	return v
}

func ClustersToGRPC(clusters []clusters.ClusterExtended) []*mongov1.Cluster {
	var v []*mongov1.Cluster
	for _, cluster := range clusters {
		v = append(v, ClusterToGRPC(cluster))
	}
	return v
}

func UserSpecFromGRPC(spec *mongov1.UserSpec) mongomodels.UserSpec {
	us := mongomodels.UserSpec{Name: spec.Name, Password: secret.NewString(spec.Password)}
	for _, perm := range spec.Permissions {
		us.Permissions = append(us.Permissions, mongomodels.Permission{
			DatabaseName: perm.DatabaseName,
			Roles:        perm.Roles,
		})
	}

	return us
}

func UserSpecsFromGRPC(specs []*mongov1.UserSpec) []mongomodels.UserSpec {
	var v []mongomodels.UserSpec
	for _, spec := range specs {
		if spec != nil {
			v = append(v, UserSpecFromGRPC(spec))
		}
	}
	return v
}

func DatabaseToGRPC(db mongomodels.Database) *mongov1.Database {
	return &mongov1.Database{
		Name:      db.Name,
		ClusterId: db.ClusterID,
	}
}

func DatabasesToGRPC(dbs []mongomodels.Database) []*mongov1.Database {
	var v []*mongov1.Database
	for _, db := range dbs {
		v = append(v, DatabaseToGRPC(db))
	}
	return v
}

func DatabaseSpecFromGRPC(spec *mongov1.DatabaseSpec) mongomodels.DatabaseSpec {
	return mongomodels.DatabaseSpec{Name: spec.Name}
}

func DatabaseSpecsFromGRPC(specs []*mongov1.DatabaseSpec) []mongomodels.DatabaseSpec {
	var v []mongomodels.DatabaseSpec
	for _, spec := range specs {
		if spec != nil {
			v = append(v, DatabaseSpecFromGRPC(spec))
		}
	}
	return v
}

func UserToGRPC(user mongomodels.User) *mongov1.User {
	var perms []*mongov1.Permission
	for _, p := range user.Permissions {
		perms = append(perms, &mongov1.Permission{
			DatabaseName: p.DatabaseName,
			Roles:        p.Roles,
		})
	}
	return &mongov1.User{
		Name:        user.Name,
		ClusterId:   user.ClusterID,
		Permissions: perms,
	}
}

func UsersToGRPC(users []mongomodels.User) []*mongov1.User {
	var v []*mongov1.User
	for _, u := range users {
		v = append(v, UserToGRPC(u))
	}
	return v
}

var aggregateByFromGRPC = map[mongov1.AggregateBy]mongomodels.ProfilerStatsColumn{
	mongov1.AggregateBy_AGGREGATE_BY_UNSPECIFIED: mongomodels.ProfilerStatsColumnCount,
	mongov1.AggregateBy_DURATION:                 mongomodels.ProfilerStatsColumnDuration,
	mongov1.AggregateBy_RESPONSE_LENGTH:          mongomodels.ProfilerStatsColumnResponseLength,
	mongov1.AggregateBy_KEYS_EXAMINED:            mongomodels.ProfilerStatsColumnKeysExamined,
	mongov1.AggregateBy_DOCUMENTS_EXAMINED:       mongomodels.ProfilerStatsColumnDocsExamined,
	mongov1.AggregateBy_DOCUMENTS_RETURNED:       mongomodels.ProfilerStatsColumnDocsReturned,
	mongov1.AggregateBy_COUNT:                    mongomodels.ProfilerStatsColumnCount,
}

var aggregationFunctionFromGRPC = map[mongov1.AggregationFunction]mongomodels.AggregationType{
	mongov1.AggregationFunction_AGGREGATION_FUNCTION_UNSPECIFIED: mongomodels.AggregationTypeUnspecified,
	mongov1.AggregationFunction_SUM:                              mongomodels.AggregationTypeSum,
	mongov1.AggregationFunction_AVG:                              mongomodels.AggregationTypeAvg,
}

var groupByFromGRPC = map[mongov1.GroupBy]mongomodels.ProfilerStatsGroupBy{
	mongov1.GroupBy_GROUP_BY_UNSPECIFIED: mongomodels.ProfilerStatsGroupByForm,
	mongov1.GroupBy_FORM:                 mongomodels.ProfilerStatsGroupByForm,
	mongov1.GroupBy_NAMESPACE:            mongomodels.ProfilerStatsGroupByNS,
	mongov1.GroupBy_HOSTNAME:             mongomodels.ProfilerStatsGroupByHostname,
	mongov1.GroupBy_USER:                 mongomodels.ProfilerStatsGroupByUser,
	mongov1.GroupBy_SHARD:                mongomodels.ProfilerStatsGroupByShard,
}

func AggregateByFromGRPC(aggregateBy mongov1.AggregateBy) mongomodels.ProfilerStatsColumn {
	return aggregateByFromGRPC[aggregateBy]
}

func AggregationFunctionFromGRPC(aggregationFunction mongov1.AggregationFunction) mongomodels.AggregationType {
	return aggregationFunctionFromGRPC[aggregationFunction]
}

func GroupByFromGRPC(groupBy []mongov1.GroupBy) []mongomodels.ProfilerStatsGroupBy {
	res := make([]mongomodels.ProfilerStatsGroupBy, 0, len(groupBy))
	for _, group := range groupBy {
		res = append(res, groupByFromGRPC[group])
	}
	return res

}

func GetProfilerStatsFromGRPC(req *mongov1.GetProfilerStatsRequest) (mongomodels.GetProfilerStatsOptions, error) {
	ret := mongomodels.GetProfilerStatsOptions{}
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return ret, err
	}

	ret.Filter = flt
	ret.FromTS = grpcapi.TimeFromGRPC(req.GetFromTime())
	ret.ToTS = grpcapi.TimeFromGRPC(req.GetToTime())
	ret.AggregateBy = AggregateByFromGRPC(req.GetAggregateBy())
	ret.AggregationFunction = AggregationFunctionFromGRPC(req.GetAggregationFunction())
	ret.GroupBy = GroupByFromGRPC(req.GetGroupBy())
	ret.RollupPeriod.Set(req.GetRollupPeriod())
	ret.TopX.Set(req.GetReturnTopX())

	ret.Limit = req.GetPageSize()
	return ret, nil
}

func GetProfilerRecsAtTimeFromGRPC(req *mongov1.GetProfilerRecsAtTimeRequest) (mongomodels.GetProfilerRecsAtTimeOptions, error) {
	ret := mongomodels.GetProfilerRecsAtTimeOptions{}

	ret.FromTS = grpcapi.TimeFromGRPC(req.GetFromTime())
	ret.ToTS = grpcapi.TimeFromGRPC(req.GetToTime())
	ret.RequestForm = req.GetRequestForm()
	ret.Hostname = req.GetHostname()

	ret.Limit = req.GetPageSize()
	return ret, nil
}

func GetProfilerTopFormsByStatFromGRPC(req *mongov1.GetProfilerTopFormsByStatRequest) (mongomodels.GetProfilerTopFormsByStatOptions, error) {
	ret := mongomodels.GetProfilerTopFormsByStatOptions{}
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return ret, err
	}

	ret.Filter = flt
	ret.FromTS = grpcapi.TimeFromGRPC(req.GetFromTime())
	ret.ToTS = grpcapi.TimeFromGRPC(req.GetToTime())
	ret.AggregateBy = AggregateByFromGRPC(req.GetAggregateBy())
	ret.AggregationFunction = AggregationFunctionFromGRPC(req.GetAggregationFunction())

	ret.Limit = req.GetPageSize()
	return ret, nil
}

func GetPossibleIndexesFromGRPC(req *mongov1.GetPossibleIndexesRequest) (mongomodels.GetPossibleIndexesOptions, error) {
	ret := mongomodels.GetPossibleIndexesOptions{}
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return ret, err
	}

	ret.Filter = flt
	ret.FromTS = grpcapi.TimeFromGRPC(req.GetFromTime())
	ret.ToTS = grpcapi.TimeFromGRPC(req.GetToTime())

	ret.Limit = req.GetPageSize()
	return ret, nil
}
