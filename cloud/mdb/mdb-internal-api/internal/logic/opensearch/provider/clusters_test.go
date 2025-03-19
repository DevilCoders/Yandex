package provider

import (
	"context"
	"encoding/json"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

type option int

const (
	OptionListHosts option = iota
)
const ClusterID = "test_cluster"

func setupDeleteHost(t *testing.T, excludes ...option) (context.Context, *openSearchFixture) {
	ctx, f := newOpenSearchFixture(t)

	f.Operator.EXPECT().ModifyOnCluster(ctx, ClusterID, clusters.TypeOpenSearch, gomock.Any(), gomock.Any()).
		DoAndReturn(func(_, _, _, fn interface{}, opts ...clusterslogic.OperatorOption) (operations.Operation, error) {
			fun := fn.(clusterslogic.ModifyOnClusterFunc)
			return fun(ctx, sessions.Session{}, f.Reader, f.Modifier, clusterslogic.NewClusterModel(
				clusters.Cluster{ClusterID: ClusterID},
				json.RawMessage("{}"),
			))
		})

	if !containsOption(excludes, OptionListHosts) {
		f.Reader.EXPECT().ListHosts(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
			Return([]hosts.HostExtended{
				masterHost("m1"),
				masterHost("m2"),
				dataHost("d1"),
				dataHost("d2"),
				dataHost("d3"),
			}, int64(0), false, nil)
	}

	f.Modifier.EXPECT().ValidateResources(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
		Return(clusterslogic.ResolvedHostGroups{}, true, nil)

	f.Modifier.EXPECT().DeleteHosts(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
		Return([]hosts.Host{dataHost("d1").Host}, nil)

	f.ExpectCreateTask(ctx, sessions.Session{}, operations.Operation{})

	return ctx, f
}

func TestOpenSearch_DeleteHosts(t *testing.T) {
	ctx, f := setupDeleteHost(t)
	defer f.Finish()

	_, err := f.OpenSearch.DeleteHosts(ctx, ClusterID, []string{"d1"})
	require.NoError(t, err, "should successfully delete data node")
}

func TestOpenSearch_DeleteHosts_ListPaging(t *testing.T) {
	ctx, f := setupDeleteHost(t, OptionListHosts)
	defer f.Finish()

	// page1
	f.Reader.EXPECT().ListHosts(ctx, ClusterID, gomock.Any(), int64(0)).
		Return([]hosts.HostExtended{masterHost("m1"), masterHost("m2")}, int64(0), true, nil)
	// page2
	f.Reader.EXPECT().ListHosts(ctx, ClusterID, gomock.Any(), int64(2)).
		Return([]hosts.HostExtended{dataHost("d1"), dataHost("d2"), dataHost("d3")}, int64(0), false, nil)

	_, err := f.OpenSearch.DeleteHosts(ctx, ClusterID, []string{"d1"})
	require.NoError(t, err, "should successfully find data node to delete")
}

func setupAddHosts(t *testing.T, excludes ...option) (context.Context, *openSearchFixture) {
	ctx, f := newOpenSearchFixture(t)

	f.Operator.EXPECT().ModifyOnCluster(ctx, ClusterID, clusters.TypeOpenSearch, gomock.Any(), gomock.Any()).
		DoAndReturn(func(_, _, _, fn interface{}, opts ...clusterslogic.OperatorOption) (operations.Operation, error) {
			fun := fn.(clusterslogic.ModifyOnClusterFunc)
			return fun(ctx, sessions.Session{}, f.Reader, f.Modifier, clusterslogic.NewClusterModel(
				clusters.Cluster{ClusterID: ClusterID},
				json.RawMessage("{}"),
			))
		})

	if !containsOption(excludes, OptionListHosts) {
		f.Reader.EXPECT().ListHosts(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
			Return([]hosts.HostExtended{
				masterHost("m1"),
				masterHost("m2"),
				dataHost("d1"),
				dataHost("d2"),
				dataHost("d3"),
			}, int64(0), false, nil)
	}

	f.Modifier.EXPECT().ValidateResources(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
		Return(clusterslogic.NewResolvedHostGroups([]clusterslogic.ResolvedHostGroup{{}}), true, nil)

	f.Compute.EXPECT().NetworkAndSubnets(gomock.Any(), gomock.Any()).
		Return(networkProvider.Network{}, nil, nil)

	f.Modifier.EXPECT().GenerateFQDN(gomock.Any(), gomock.Any(), gomock.Any()).
		Return("newhost", nil)

	f.Compute.EXPECT().PickSubnet(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
		Return(networkProvider.Subnet{}, nil)

	f.Modifier.EXPECT().AddHosts(gomock.Any(), gomock.Any()).
		Return([]hosts.Host{{}}, nil)

	f.ExpectCreateTask(ctx, sessions.Session{}, operations.Operation{})

	return ctx, f
}

func TestOpenSearch_AddHosts(t *testing.T) {
	ctx, f := setupAddHosts(t)
	defer f.Finish()

	_, err := f.OpenSearch.AddHosts(ctx, ClusterID, []osmodels.Host{{Role: hosts.RoleOpenSearchDataNode}})
	require.NoError(t, err, "should successfully add data node")
}

func TestOpenSearch_AddHosts_ListPaging(t *testing.T) {
	ctx, f := setupAddHosts(t, OptionListHosts)
	defer f.Finish()

	// page1
	f.Reader.EXPECT().ListHosts(ctx, ClusterID, gomock.Any(), int64(0)).
		Return([]hosts.HostExtended{masterHost("m1"), masterHost("m2")}, int64(0), true, nil)
	// page2
	f.Reader.EXPECT().ListHosts(ctx, ClusterID, gomock.Any(), int64(2)).
		Return([]hosts.HostExtended{dataHost("d1"), dataHost("d2"), dataHost("d3")}, int64(0), false, nil)

	_, err := f.OpenSearch.AddHosts(ctx, ClusterID, []osmodels.Host{{Role: hosts.RoleOpenSearchDataNode}})
	require.NoError(t, err, "should successfully add data node")
}

func containsOption(a []option, value option) bool {
	for i := range a {
		if a[i] == value {
			return true
		}
	}
	return false
}

func masterHost(fqdn string) hosts.HostExtended {
	return hosts.HostExtended{Host: hosts.Host{
		FQDN:  fqdn,
		Roles: []hosts.Role{hosts.RoleOpenSearchMasterNode},
	}}
}

func dataHost(fqdn string) hosts.HostExtended {
	return hosts.HostExtended{Host: hosts.Host{
		FQDN:  fqdn,
		Roles: []hosts.Role{hosts.RoleOpenSearchDataNode},
	}}
}
