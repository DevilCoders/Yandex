package hostpicker

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"

	metadbMocks "a.yandex-team.ru/cloud/mdb/backup/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestReplicaHostPickerUsesBackupPriority(t *testing.T) {
	lg, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		assert.Error(t, err)
		return
	}

	mockCtrl := gomock.NewController(lg)
	defer mockCtrl.Finish()

	health := setupHealthServiceMock(mockCtrl)

	mdbMock := setupMdbMock(mockCtrl, map[string]string{
		"master":   "100",
		"replica1": "1",
		"replica2": "2",
		"replica3": "3",
	})

	hp := NewPreferReplicaHostPicker(mdbMock, health, lg, DefaultConfig().DefaultSettings, "service_health", PriorityArgs{
		PillarPath: []string{"data", "mysql", "backup_priority"},
	})

	assert.NotNil(t, hp)

	tests := []struct {
		name           string
		hosts          []string
		expectedResult string
	}{
		{"Select replica with highest priority",
			[]string{
				"master",
				"replica1",
				"replica2",
				"replica3",
			},
			"replica3"},
		{"Select single replica",
			[]string{
				"master",
				"replica1",
			},
			"replica1"},
		{"Select single master",
			[]string{
				"master",
			},
			"master"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			host, err := hp.PickHost(context.TODO(), tt.hosts)

			if assert.NoError(t, err) {
				assert.Equal(t, tt.expectedResult, host)
			}
		})
	}
}

func setupHealthServiceMock(mockCtrl *gomock.Controller) *mocks.MockMDBHealthClient {
	health := mocks.NewMockMDBHealthClient(mockCtrl)

	health.EXPECT().
		GetHostsHealth(gomock.Any(), gomock.Any()).
		AnyTimes().
		DoAndReturn(func(_ context.Context, fqdns []string) ([]types.HostHealth, error) {
			var ret []types.HostHealth
			for i, f := range fqdns {
				role := types.ServiceRoleMaster
				if i > 0 {
					role = types.ServiceRoleReplica
				}
				repl := types.NewServiceHealth("service_health", time.Now(), types.ServiceStatusAlive, role, types.ServiceReplicaTypeUnknown, "", 0, nil)
				ret = append(ret, types.NewHostHealth(
					"cid",
					f,
					[]types.ServiceHealth{repl},
				))
			}
			return ret, nil
		})

	return health
}

func setupMdbMock(mockCtrl *gomock.Controller, pillarData map[string]string) *metadbMocks.MockMetaDB {
	mdbMock := metadbMocks.NewMockMetaDB(mockCtrl)

	mdbMock.EXPECT().
		HostsPillarByPath(gomock.Any(), gomock.Any(), gomock.Any()).
		AnyTimes().
		DoAndReturn(func(ctx context.Context, fqdns []string, path []string) (map[string]string, error) {
			result := make(map[string]string)

			for _, fqdn := range fqdns {
				result[fqdn] = pillarData[fqdn]
			}

			return result, nil
		})

	return mdbMock
}
