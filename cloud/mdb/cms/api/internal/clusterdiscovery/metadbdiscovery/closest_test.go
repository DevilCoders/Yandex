package metadbdiscovery

import (
	"context"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadb_mocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

func TestMetaDBBasedDiscovery_FindInShardOrSubcidByFQDN(t *testing.T) {

	type input struct {
		fqdn    string
		prepare func(db *metadb_mocks.MockMetaDB)
	}
	type output struct {
		result  clusterdiscovery.Neighbourhood
		withErr string
	}
	type testCase struct {
		name   string
		input  input
		expect output
	}
	fqdn := "man1.db.yandex.net"

	tcs := []testCase{
		{
			name: "not found by fqdn",
			input: input{
				fqdn: fqdn,
				prepare: func(db *metadb_mocks.MockMetaDB) {
					db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
					db.EXPECT().Rollback(gomock.Any()).Return(nil)

					db.EXPECT().GetHostByFQDN(gomock.Any(), fqdn).Return(
						metadb.Host{}, metadb.ErrDataNotFound)
				},
			},
			expect: output{
				withErr: fmt.Sprintf("host %s does not exist", fqdn),
			},
		}, {
			name: "2 found by shard",
			input: input{
				fqdn: fqdn,
				prepare: func(db *metadb_mocks.MockMetaDB) {
					shardID := "1"
					db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
					db.EXPECT().Commit(gomock.Any()).Return(nil)
					db.EXPECT().Rollback(gomock.Any()).Return(nil)

					db.EXPECT().GetHostByFQDN(gomock.Any(), fqdn).Return(
						metadb.Host{
							FQDN:    fqdn,
							ShardID: optional.NewString(shardID),
						}, nil)
					db.EXPECT().GetHostsByShardID(gomock.Any(), shardID).Return(
						[]metadb.Host{
							{FQDN: "man2.db.yandex.net"},
							{FQDN: fqdn},
						}, nil)
				},
			},
			expect: output{
				result: clusterdiscovery.Neighbourhood{
					Others: []metadb.Host{
						{FQDN: "man2.db.yandex.net"},
					},
					Self: metadb.Host{
						FQDN: fqdn,
					},
				},
			},
		}, {
			name: "2 found by subcid",
			input: input{
				fqdn: fqdn,
				prepare: func(db *metadb_mocks.MockMetaDB) {
					db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
					db.EXPECT().Commit(gomock.Any()).Return(nil)
					db.EXPECT().Rollback(gomock.Any()).Return(nil)
					subCid := "1"
					db.EXPECT().GetHostByFQDN(gomock.Any(), fqdn).Return(
						metadb.Host{
							FQDN:         fqdn,
							SubClusterID: subCid,
						}, nil)
					db.EXPECT().GetHostsBySubcid(gomock.Any(), subCid).Return(
						[]metadb.Host{
							{FQDN: fqdn},
							{FQDN: "man2.db.yandex.net"},
						}, nil)
				},
			},
			expect: output{
				result: clusterdiscovery.Neighbourhood{
					Others: []metadb.Host{
						{FQDN: "man2.db.yandex.net"},
					},
					Self: metadb.Host{
						FQDN: fqdn,
					},
				},
			},
		},
	}
	for _, tc := range tcs {
		ctx := context.Background()
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			mock := metadb_mocks.NewMockMetaDB(ctrl)
			tc.input.prepare(mock)
			dscvry := NewMetaDBBasedDiscovery(mock)
			result, err := dscvry.FindInShardOrSubcidByFQDN(ctx, tc.input.fqdn)
			if tc.expect.withErr == "" {
				require.NoError(t, err)
				require.Equal(t, tc.expect.result.Others, result.Others)
				require.Equal(t, tc.expect.result.Self, result.Self)
			} else {
				require.EqualError(t, err, tc.expect.withErr)
			}
		})
	}
}
