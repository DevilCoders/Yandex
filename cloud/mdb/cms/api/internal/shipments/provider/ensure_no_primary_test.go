package provider

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
)

func TestShipmentProvider_ensureFromCMD(t *testing.T) {
	type testCase struct {
		name     string
		input    []string
		expected []deployModels.CommandDef
	}

	testCases := []testCase{
		{
			name:  "one host",
			input: []string{"fqdn1"},
			expected: []deployModels.CommandDef{{
				Type: "cmd.run",
				Args: []string{"if test -f /usr/local/yandex/ensure_no_primary.sh ; then " +
					"/usr/local/yandex/ensure_no_primary.sh --from-hosts=fqdn1; fi"},
				Timeout: encodingutil.Duration{Duration: 20 * time.Minute},
			}},
		},
		{
			name:  "two hosts",
			input: []string{"fqdn1", "fqdn2"},
			expected: []deployModels.CommandDef{{
				Type: "cmd.run",
				Args: []string{"if test -f /usr/local/yandex/ensure_no_primary.sh ; then " +
					"/usr/local/yandex/ensure_no_primary.sh --from-hosts=fqdn1,fqdn2; fi"},
				Timeout: encodingutil.Duration{Duration: 20 * time.Minute},
			}},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			provider := NewShipmentProvider(nil, nil)
			result := provider.ensureFromCMD(tc.input)
			require.Equal(t, tc.expected, result)
		})
	}
}

func TestShipmentProvider_sameRegionClusterLegsByFQDN(t *testing.T) {
	fqdn := "fqdn1.f.q.d.n"
	type testCase struct {
		name           string
		expected       []string
		prepareFactory func(mDB *metadbmocks.MockMetaDB)
	}

	testCases := []testCase{
		{
			name:     "one host",
			expected: []string{"fqdn1"},
			prepareFactory: func(mDB *metadbmocks.MockMetaDB) {
				mDB.EXPECT().GetHostsBySubcid(gomock.Any(), "asd").Return(
					[]metadb.Host{
						{
							FQDN: fqdn,
							Geo:  "geo1",
						},
						{
							FQDN: "fqdn2",
							Geo:  "geo2",
						},
					}, nil,
				)
			},
		},
		{
			name:     "two hosts",
			expected: []string{"fqdn1", "fqdn2"},
			prepareFactory: func(mDB *metadbmocks.MockMetaDB) {
				mDB.EXPECT().GetHostsBySubcid(gomock.Any(), "asd").Return(
					[]metadb.Host{
						{
							FQDN: fqdn,
							Geo:  "geo1",
						},
						{
							FQDN: "fqdn2.f.q",
							Geo:  "geo1",
						},
					}, nil,
				)
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctx, matcher := testutil.MatchContext(t, ctx)
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			mDB := metadbmocks.NewMockMetaDB(ctrl)
			mDB.EXPECT().Begin(matcher, gomock.Any()).Return(ctx, nil)
			mDB.EXPECT().Rollback(matcher).Return(nil)
			mDB.EXPECT().Commit(matcher).Return(nil)
			mDB.EXPECT().GetHostByFQDN(gomock.Any(), fqdn).Return(
				metadb.Host{
					FQDN:         fqdn,
					SubClusterID: "asd",
				}, nil,
			)

			tc.prepareFactory(mDB)

			provider := NewShipmentProvider(nil, mDB)
			result, err := provider.sameRegionClusterLegsByFQDN(ctx, fqdn)
			require.NoError(t, err)
			require.Equal(t, tc.expected, result)
		})
	}
}
