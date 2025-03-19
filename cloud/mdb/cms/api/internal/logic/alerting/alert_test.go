package alerting

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	cms_models "a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestFilterOutAlarming(t *testing.T) {
	ctx := context.Background()
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
	type args struct {
		operations      []cms_models.ManagementInstanceOperation
		cfg             AlertConfig
		knownConditions map[ConditionType]condHandler
	}
	now, err := time.Parse(time.RFC1123, time.RFC1123)
	if err != nil {
		panic(err)
	}
	tests := []struct {
		name string
		args args
		want []cms_models.ManagementInstanceOperation
	}{
		{
			name: "operation matches type and stays in result",
			args: args{
				operations: []cms_models.ManagementInstanceOperation{
					{
						ID: "test-op-id",
					},
				},
				cfg: AlertConfig{
					Conditions: []AlarmCondition{
						{
							ConditionType: ConditionByStepName,
						},
					},
				},
				knownConditions: map[ConditionType]condHandler{
					ConditionByStepName: {
						matchesType: func(ctx context.Context, o cms_models.ManagementInstanceOperation, condition AlarmCondition) (bool, error) {
							return true, nil
						},
						keepInResult: func(o cms_models.ManagementInstanceOperation, condition AlarmCondition, now time.Time) bool {
							return true
						},
					},
				},
			},
			want: []cms_models.ManagementInstanceOperation{
				{
					ID: "test-op-id",
				},
			},
		},
		{
			name: "operation matches type and filters out",
			args: args{
				operations: []cms_models.ManagementInstanceOperation{
					{
						ID: "test-op-id",
					},
				},
				cfg: AlertConfig{
					Conditions: []AlarmCondition{
						{
							ConditionType: ConditionByStepName,
						},
					},
				},
				knownConditions: map[ConditionType]condHandler{
					ConditionByStepName: {
						matchesType: func(ctx context.Context, o cms_models.ManagementInstanceOperation, condition AlarmCondition) (bool, error) {
							return true, nil
						},
						keepInResult: func(o cms_models.ManagementInstanceOperation, condition AlarmCondition, now time.Time) bool {
							return false
						},
					},
				},
			},
			want: []cms_models.ManagementInstanceOperation(nil),
		},
		{
			name: "operation matches none and stays",
			args: args{
				operations: []cms_models.ManagementInstanceOperation{
					{
						ID: "test-op-id",
					},
				},
				cfg: AlertConfig{
					Conditions: []AlarmCondition{
						{
							ConditionType: ConditionByStepName,
						},
					},
				},
				knownConditions: map[ConditionType]condHandler{},
			},
			want: []cms_models.ManagementInstanceOperation{
				{
					ID: "test-op-id",
				},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := FilterOutAlertingOps(ctx, logger, tt.args.operations, tt.args.cfg, now, tt.args.knownConditions)
			require.NoError(t, err)
			require.Equal(t, tt.want, got)
		})
	}
}
