package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestInstanceToFqdnConverter(t *testing.T) {
	type testCase struct {
		name           string
		prepareMeta    func(db *metadbmocks.MockMetaDB)
		expected       steps.RunResult
		stepCtxFactory func() *opcontext.OperationContext
		isCompute      bool
		dom0FQDN       string
	}

	testCases := []testCase{
		{
			name: "get fqdn by id",
			prepareMeta: func(db *metadbmocks.MockMetaDB) {
				db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
				db.EXPECT().Rollback(gomock.Any()).Return(nil)
				db.EXPECT().GetHostByVtypeID(gomock.Any(), "fqdn1").Return(
					metadb.Host{FQDN: "qwerty"},
					nil,
				)
			},
			stepCtxFactory: func() *opcontext.OperationContext {
				return opcontext.NewStepContext(models.ManagementInstanceOperation{
					ID:         "qwe",
					InstanceID: "fqdn1",
					State:      models.DefaultOperationState(),
				})
			},
			expected: steps.RunResult{
				Error:       nil,
				Description: "got fqdn \"qwerty\"",
				IsDone:      true,
			},
			isCompute: true,
		},
		{
			name:        "fqdn in state",
			prepareMeta: func(db *metadbmocks.MockMetaDB) {},
			stepCtxFactory: func() *opcontext.OperationContext {
				state := models.DefaultOperationState()
				state.FQDN = "anotherfqdn"
				return opcontext.NewStepContext(models.ManagementInstanceOperation{
					ID:         "qwe",
					InstanceID: "fqdn1",
					State:      state,
				})
			},
			expected: steps.RunResult{
				Error:       nil,
				Description: "got fqdn \"anotherfqdn\"",
				IsDone:      true,
			},
			isCompute: true,
		},
		{
			name: "unknown instance ID",
			prepareMeta: func(db *metadbmocks.MockMetaDB) {
				db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
				db.EXPECT().Rollback(gomock.Any()).Return(nil)
				db.EXPECT().GetHostByVtypeID(gomock.Any(), "fqdn1").Return(
					metadb.Host{},
					metadb.ErrDataNotFound,
				)
			},
			stepCtxFactory: func() *opcontext.OperationContext {
				return opcontext.NewStepContext(models.ManagementInstanceOperation{
					ID:         "qwe",
					InstanceID: "fqdn1",
					State:      models.DefaultOperationState(),
				})
			},
			expected: steps.RunResult{
				Error:       xerrors.New("unknown instanceID \"fqdn1\""),
				Description: "unknown instanceID \"fqdn1\"",
				IsDone:      true,
			},
			isCompute: true,
		},
		{
			name: "can't get fqdn",
			prepareMeta: func(db *metadbmocks.MockMetaDB) {
				db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
				db.EXPECT().Rollback(gomock.Any()).Return(nil)
				db.EXPECT().GetHostByVtypeID(gomock.Any(), "fqdn1").Return(
					metadb.Host{},
					xerrors.New("some error"),
				)
			},
			stepCtxFactory: func() *opcontext.OperationContext {
				return opcontext.NewStepContext(models.ManagementInstanceOperation{
					ID:         "qwe",
					InstanceID: "fqdn1",
					State:      models.DefaultOperationState(),
				})
			},
			expected: steps.RunResult{
				Error:       xerrors.New("some error"),
				Description: "can't get host info from metadb",
				IsDone:      false,
			},
			isCompute: true,
		},
		{
			name: "wrong porto format",
			prepareMeta: func(db *metadbmocks.MockMetaDB) {
			},
			stepCtxFactory: func() *opcontext.OperationContext {
				return opcontext.NewStepContext(models.ManagementInstanceOperation{
					ID:         "qwe",
					InstanceID: "fqdn1",
					State:      models.DefaultOperationState(),
				})
			},
			expected: steps.RunResult{
				Error:       xerrors.New("wrong instance ID format for porto installation, it must be 'dom0_fqdn:container_fqdn', got \"fqdn1\""),
				Description: "wrong instance ID format for porto installation, it must be 'dom0_fqdn:container_fqdn', got \"fqdn1\"",
				IsDone:      true,
			},
			isCompute: false,
		},
		{
			name: "porto get fqdn",
			prepareMeta: func(db *metadbmocks.MockMetaDB) {
			},
			stepCtxFactory: func() *opcontext.OperationContext {
				return opcontext.NewStepContext(models.ManagementInstanceOperation{
					ID:         "qwe",
					InstanceID: "dom0:fqdn1",
					State:      models.DefaultOperationState(),
				})
			},
			expected: steps.RunResult{
				Error:       nil,
				Description: "got fqdn \"fqdn1\"",
				IsDone:      true,
			},
			isCompute: false,
			dom0FQDN:  "dom0",
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			mdb := metadbmocks.NewMockMetaDB(ctrl)
			tc.prepareMeta(mdb)

			step := steps.NewInstanceToFqdnConverterStep(mdb, tc.isCompute)

			stepCtx := tc.stepCtxFactory()
			testStep(t, ctx, stepCtx, step, tc.expected)
			assert.Equal(t, tc.dom0FQDN, stepCtx.Dom0FQDN())
		})
	}
}
