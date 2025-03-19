package katan_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/katan"
)

func TestSentryTagsFromRollout(t *testing.T) {
	tests := []struct {
		name    string
		rollout katandb.Rollout
		want    map[string]string
	}{
		{
			"without schedule_id",
			katandb.Rollout{ID: 42},
			map[string]string{"rollout_id": "42"},
		},
		{
			"with schedule_id",
			katandb.Rollout{ID: 42, ScheduleID: optional.NewInt64(10)},
			map[string]string{"rollout_id": "42", "schedule_id": "10"},
		},
		{
			"with created_by",

			katandb.Rollout{ID: 42, CreatedBy: "root"},
			map[string]string{"rollout_id": "42", "user": "root"},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.want, katan.SentryTagsFromRollout(tt.rollout))
		})
	}
}
