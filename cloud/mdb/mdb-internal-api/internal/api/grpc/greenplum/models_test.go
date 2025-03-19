package greenplum

import (
	"testing"

	"github.com/stretchr/testify/require"
	"google.golang.org/genproto/protobuf/field_mask"

	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
)

func TestClusterModifyArgsFromGRPC(t *testing.T) {
	tests := []struct {
		name    string
		request *gpv1.UpdateClusterRequest
		result  *greenplum.ModifyClusterArgs
	}{
		{
			name: "access settings",
			request: &gpv1.UpdateClusterRequest{
				ClusterId: "cid1",
				Config: &gpv1.GreenplumConfig{
					Access: &gpv1.Access{
						DataLens:     true,
						WebSql:       true,
						DataTransfer: true,
						Serverless:   true,
					},
				},
				UpdateMask: &field_mask.FieldMask{
					Paths: []string{
						"config.access.data_lens",
						"config.access.web_sql",
						"config.access.data_transfer",
						"config.access.serverless",
					},
				},
			},
			result: func() *greenplum.ModifyClusterArgs {
				result := greenplum.ModifyClusterArgs{}
				result.ClusterID = "cid1"
				result.Access.DataLens = optional.NewBool(true)
				result.Access.WebSQL = optional.NewBool(true)
				result.Access.DataTransfer = optional.NewBool(true)
				result.Access.Serverless = optional.NewBool(true)
				return &result
			}(),
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			args, err := clusterModifyArgsFromGRPC(test.request)
			require.NoError(t, err)
			require.NotNil(t, args)
			require.Equal(t, args, *test.result)
		})
	}
}
