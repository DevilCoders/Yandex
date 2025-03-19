package grpcserver_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/conductorcache"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/grpcserver"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/conductor/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestDoRefreshConductorCache(t *testing.T) {
	tcs := []struct {
		fqdn     string
		expected []string
		unknown  bool
	}{
		{
			fqdn:     "fqdn1",
			expected: []string{"group1", "group3"},
		},
		{
			fqdn:     "fqdn2",
			expected: []string{"group2", "group3", "group4", "group5"},
		},
	}
	data := conductor.ExecuterData{
		Hosts: map[string]conductor.HostExecuterData{
			"fqdn1": {Group: "group1"},
			"fqdn2": {Group: "group2"},
		},
		Groups: map[string]conductor.GroupExecuterData{
			"group1": {Parents: []string{"group3"}},
			"group2": {Parents: []string{"group3", "group4"}},
			"group3": {Parents: nil},
			"group4": {Parents: []string{"group5"}},
			"group5": {Parents: nil},
		},
	}

	ctx := context.Background()
	cntrl := gomock.NewController(t)
	defer cntrl.Finish()
	cndcl := mocks.NewMockClient(cntrl)
	cndcl.EXPECT().ExecuterData(gomock.Any(), gomock.Any()).Return(data, nil).Times(1)

	cache := conductorcache.NewCache()
	service := grpcserver.NewInstanceService(nil, nil, nil, nil, &nop.Logger{}, nil, cndcl, cache, grpcserver.DefaultConfig())
	require.NoError(t, service.DoRefreshConductorCache(ctx))
	for _, tc := range tcs {
		t.Run("", func(t *testing.T) {
			groups, ok := cache.HostGroups(tc.fqdn)
			if tc.unknown {
				require.False(t, ok)
			} else {
				require.True(t, ok)
				require.Equal(t, tc.expected, groups)
			}
		})
	}
}
