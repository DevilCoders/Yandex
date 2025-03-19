package steps_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	jugglerservice "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	dbmmock "a.yandex-team.ru/cloud/mdb/internal/dbm/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	jugglermock "a.yandex-team.ru/cloud/mdb/internal/juggler/mocks"
)

func TestGroupClusterNodesByUnreachableCheck(t *testing.T) {
	const (
		otherNodeHostname  = "test1.net"
		testedNodeHostname = "tested-node.net"
		testedCluster      = "tested-cluster"
	)
	type GroupTC struct {
		input    []juggler.RawEvent
		expected jugglerservice.FQDNGroupByJugglerCheck
	}
	tcs := []GroupTC{
		{
			[]juggler.RawEvent{},
			jugglerservice.FQDNGroupByJugglerCheck{
				NotOK: []string{otherNodeHostname},
			},
		},
		{
			[]juggler.RawEvent{
				{Status: "CRIT", ReceivedTime: time.Now(), Host: otherNodeHostname, Service: "UNREACHABLE"},
				{Status: "OK", ReceivedTime: time.Now().Add(-time.Hour), Host: otherNodeHostname, Service: "UNREACHABLE"},
			},
			jugglerservice.FQDNGroupByJugglerCheck{
				NotOK: []string{otherNodeHostname},
			},
		},
		{
			[]juggler.RawEvent{
				{Status: "CRIT", ReceivedTime: time.Now().Add(-time.Hour), Host: otherNodeHostname, Service: "UNREACHABLE"},
				{Status: "OK", ReceivedTime: time.Now(), Host: otherNodeHostname, Service: "UNREACHABLE"},
			},
			jugglerservice.FQDNGroupByJugglerCheck{
				OK: []string{otherNodeHostname},
			},
		},
	}
	for index, tc := range tcs {
		t.Run(fmt.Sprintf("%d", index), func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			dbmClient := dbmmock.NewMockClient(ctrl)
			dbmClient.EXPECT().ClusterContainers(gomock.Any(), testedCluster).Return([]dbm.Container{
				{
					ClusterName: testedCluster,
					FQDN:        otherNodeHostname,
				},
				{
					FQDN:        testedNodeHostname,
					ClusterName: testedCluster,
				},
			}, nil)
			jugglerClient := jugglermock.NewMockAPI(ctrl)
			jugglerClient.EXPECT().RawEvents(gomock.Any(), []string{otherNodeHostname}, []string{"UNREACHABLE"}).Return(tc.input, nil)
			s := steps.NewMetadataOnClusterNodesStep(nil, nil, shipments.AwaitShipmentConfig{}, jugglerservice.DefaultJugglerConfig(), jugglerClient, dbmClient)

			runRes, err := s.(*steps.MetadataOnClusterNodesStep).GroupClusterNodesByUnreachableCheck(ctx, []models.Instance{
				{DBMClusterName: testedCluster, FQDN: testedNodeHostname},
			})

			require.NoError(t, err)
			require.Equal(t, tc.expected, runRes)
		})
	}

}
