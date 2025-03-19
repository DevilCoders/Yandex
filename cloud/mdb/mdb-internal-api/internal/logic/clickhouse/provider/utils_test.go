package provider

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestClickHouse_SelectZones(t *testing.T) {
	ctx := context.Background()
	session := sessions.Session{}
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	ctrl := gomock.NewController(l)
	creator := mocks.NewMockCreator(ctrl)
	creator.EXPECT().ListAvailableZones(gomock.Any(), gomock.Any(), gomock.Any()).Return([]environment.Zone{
		{Name: "man"}, {Name: "vla"}, {Name: "sas", RegionID: "region1"},
	}, nil).AnyTimes()

	t.Run("Select all available", func(t *testing.T) {
		zones, err := selectHostZones(ctx, session, creator, 3, nil, optional.String{})
		require.NoError(t, err)
		require.ElementsMatch(t, zones, []string{"sas", "vla", "man"})
	})

	t.Run("Select only preferred", func(t *testing.T) {
		zones, err := selectHostZones(ctx, session, creator, 2, []string{"iva", "myt"}, optional.String{})
		require.NoError(t, err)
		require.ElementsMatch(t, zones, []string{"iva", "myt"})
	})

	t.Run("Select preferred and available", func(t *testing.T) {
		zones, err := selectHostZones(ctx, session, creator, 5, []string{"iva", "myt"}, optional.String{})
		require.NoError(t, err)
		require.Len(t, zones, 5)
		require.ElementsMatch(t, zones, []string{"sas", "vla", "man", "iva", "myt"})
	})

	t.Run("Select preferred from available", func(t *testing.T) {
		zones, err := selectHostZones(ctx, session, creator, 2, []string{"sas", "man"}, optional.String{})
		require.NoError(t, err)
		require.Len(t, zones, 2)
		require.ElementsMatch(t, zones, []string{"sas", "man"})
	})

	t.Run("Select preferred twice", func(t *testing.T) {
		zones, err := selectHostZones(ctx, session, creator, 4, []string{"sas"}, optional.String{})
		require.NoError(t, err)
		require.ElementsMatch(t, zones, []string{"sas", "sas", "man", "vla"})
	})

	t.Run("Select available multiple", func(t *testing.T) {
		zones, err := selectHostZones(ctx, session, creator, 6, nil, optional.String{})
		require.NoError(t, err)
		require.Len(t, zones, 6)
		require.Contains(t, zones, "sas")
		require.Contains(t, zones, "vla")
		require.Contains(t, zones, "man")
	})

	t.Run("Select last available except preferred", func(t *testing.T) {
		zones, err := selectHostZones(ctx, session, creator, 3, []string{"man", "vla"}, optional.String{})
		require.NoError(t, err)
		require.Len(t, zones, 3)
		require.ElementsMatch(t, zones, []string{"man", "vla", "sas"})
	})

	t.Run("Select only from region", func(t *testing.T) {
		zones, err := selectHostZones(ctx, session, creator, 3, []string{}, optional.NewString("region1"))
		require.NoError(t, err)
		require.Len(t, zones, 3)
		require.ElementsMatch(t, zones, []string{"sas", "sas", "sas"})
	})
}
