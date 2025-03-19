package grpcserver_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc/metadata"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/grpcserver"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/stub"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	mdbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func TestMoveInstance(t *testing.T) {
	type testData struct {
		name       string
		instanceID string
		dType      metadb.DiskType
		isCompute  bool
		forceMove  bool
		err        func(error) bool
	}

	existingComputeInstanceID := "instance_id_existing"
	missingComputeInstanceID := "instance_id_missing"
	wrongDiskTypeComputeInstanceID := "wrong_disk_type"

	existingPortoFQDN := "fqdn_existing"
	missingPortoFQDN := "fqdn_missing"
	existingPortoInstanceID := fmt.Sprintf("dom0:%s", existingPortoFQDN)
	missingPortoInstanceID := fmt.Sprintf("dom0:%s", missingPortoFQDN)
	incompletePortoInstanceID := existingPortoFQDN

	testOpID := "qwe"
	testComment := "testing"
	testAuthor := "author"
	testToken := "qwerty"
	testFolderID := "fol1"

	testCases := []testData{
		{
			name:       "simple compute",
			instanceID: existingComputeInstanceID,
			dType:      metadb.LocalSSD,
			isCompute:  true,
		},
		{
			name:       "missing compute instance id",
			instanceID: missingComputeInstanceID,
			dType:      metadb.LocalSSD,
			isCompute:  true,
			err:        semerr.IsNotFound,
		},
		{
			name:       "not a local-ssd disk in compute",
			instanceID: wrongDiskTypeComputeInstanceID,
			dType:      metadb.NetworkSSD,
			isCompute:  true,
			err:        semerr.IsFailedPrecondition,
		},
		{
			name:       "not a local-ssd disk in compute with force move",
			instanceID: wrongDiskTypeComputeInstanceID,
			dType:      metadb.NetworkSSD,
			isCompute:  true,
			forceMove:  true,
		},
		{
			name:       "simple porto",
			instanceID: existingPortoInstanceID,
		},
		{
			name:       "missing porto fqdn in instance id",
			instanceID: missingPortoInstanceID,
			err:        semerr.IsNotFound,
		},
		{
			name:       "wrong format porto",
			instanceID: incompletePortoInstanceID,
			err:        semerr.IsInvalidInput,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			idem, err := idempotence.New()
			require.NoError(t, err)
			ctx = idempotence.WithIncoming(ctx, idempotence.Incoming{ID: idem})
			ctx = metadata.NewIncomingContext(ctx, metadata.New(map[string]string{"authorization": fmt.Sprintf("Bearer %s", testToken)}))
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			cms := mocks.NewMockClient(ctrl)
			cms.EXPECT().CreateInstanceOperation(
				ctx,
				idem,
				models.InstanceOperationMove,
				tc.instanceID,
				testComment,
				testAuthor,
			).AnyTimes()
			cms.EXPECT().GetInstanceOperation(ctx, gomock.Any()).Return(models.ManagementInstanceOperation{
				ID:          testOpID,
				Type:        models.InstanceOperationMove,
				Status:      models.InstanceOperationStatusInProgress,
				Comment:     testComment,
				Author:      models.Person(testAuthor),
				InstanceID:  tc.instanceID,
				Explanation: "tbd",
			}, nil).AnyTimes()

			metadbHost := metadb.Host{
				FQDN:         "fqdn",
				ClusterID:    "",
				SubClusterID: "",
				ShardID:      optional.String{},
				Geo:          "",
				Roles:        nil,
				CreatedAt:    time.Time{},
				DType:        tc.dType,
			}

			mdb := mdbmocks.NewMockMetaDB(ctrl)
			mdb.EXPECT().GetHostByVtypeID(ctx, existingComputeInstanceID).Return(
				metadbHost, nil).MaxTimes(1)
			mdb.EXPECT().GetHostByVtypeID(ctx, missingComputeInstanceID).Return(
				metadb.Host{}, metadb.ErrDataNotFound).MaxTimes(1)
			mdb.EXPECT().GetHostByVtypeID(ctx, wrongDiskTypeComputeInstanceID).Return(
				metadbHost, nil).MaxTimes(1)
			mdb.EXPECT().GetHostByFQDN(ctx, existingPortoFQDN).Return(
				metadbHost, nil).MaxTimes(1)
			mdb.EXPECT().GetHostByFQDN(ctx, missingPortoFQDN).Return(
				metadb.Host{}, metadb.ErrDataNotFound).MaxTimes(1)

			asCli := &stub.AccessService{
				Mapping: map[string]stub.Data{
					testToken: {
						Subject: as.Subject{
							User: &as.UserAccount{ID: testAuthor},
						},
						Folders: map[string]stub.PermissionSet{
							testFolderID: {},
						},
					},
				},
			}

			cfg := grpcserver.DefaultConfig()
			cfg.IsCompute = tc.isCompute
			s := grpcserver.NewInstanceService(
				cms,
				nil,
				mdb,
				nil,
				nil,
				grpcserver.NewAuth(
					asCli,
					as.ResourceFolder(testFolderID),
					"",
					false,
				),
				nil,
				nil,
				cfg,
			)
			res, err := s.MoveInstance(ctx, &api.MoveInstanceRequest{
				InstanceId: tc.instanceID,
				Comment:    testComment,
				Force:      tc.forceMove,
			})

			if tc.err == nil {
				require.NoError(t, err)
				require.Equal(t, testOpID, res.Id)
				require.Equal(t, api.InstanceOperation_PROCESSING, res.Status)
			} else {
				require.True(t, tc.err(err))
			}
		})
	}
}
