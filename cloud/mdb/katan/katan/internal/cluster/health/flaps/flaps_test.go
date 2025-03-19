package flaps

import (
	"reflect"
	"testing"
	"time"

	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
)

func TestFlappingHealthWindowValues_Verdict(t *testing.T) {
	t1, _ := time.Parse(time.RFC850, "Monday, 02-Jan-06 15:04:05 MSK")
	t2, _ := time.Parse(time.RFC850, "Monday, 02-Jan-06 15:04:15 MSK")
	t3, _ := time.Parse(time.RFC850, "Monday, 02-Jan-06 15:04:35 MSK")
	type fields struct {
		history                 []healthapi.ClusterHealth
		numStatusesInFullWindow int
	}
	tests := []struct {
		name   string
		fields fields
		want   Verdict
	}{
		{
			name: "flapping verdict alive on full window",
			fields: fields{
				history: []healthapi.ClusterHealth{
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t1,
					},
					{
						Status:    healthapi.ClusterStatusDegraded,
						Timestamp: t2,
					},
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t3,
					},
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t3.Add(time.Second * 10).Add(time.Microsecond),
					},
				},
				numStatusesInFullWindow: 3,
			},
			want: Verdict{
				IsFlapping:     true,
				Status:         healthapi.ClusterStatusAlive,
				Interpretation: "15:04:05 Alive -> +10s Degraded -> +20s Alive -> +10s Alive",
			},
		},
		{
			name: "stable verdict alive on full window",
			fields: fields{
				history: []healthapi.ClusterHealth{
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t1,
					},
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t2,
					},
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t3,
					},
				},
				numStatusesInFullWindow: 3,
			},
			want: Verdict{
				IsFlapping: false,
				Status:     healthapi.ClusterStatusAlive,
			},
		},
		{
			name: "flapping verdict unknown on non full window",
			fields: fields{
				history: []healthapi.ClusterHealth{
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t1,
					},
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t2,
					},
				},
				numStatusesInFullWindow: 3,
			},
			want: Verdict{
				IsFlapping:     true,
				Status:         healthapi.ClusterStatusUnknown,
				Interpretation: "only 2 measurements were made, 3 required",
			},
		},
		{
			name: "repeated responses are considered same",
			fields: fields{
				history: []healthapi.ClusterHealth{
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t1,
					},
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t1,
					},
					{
						Status:    healthapi.ClusterStatusDegraded,
						Timestamp: t1,
					},
				},
				numStatusesInFullWindow: 3,
			},
			want: Verdict{
				IsFlapping:     true,
				Status:         healthapi.ClusterStatusUnknown,
				Interpretation: "only 1 measurements were made, 3 required",
			},
		},
		{
			name: "verdict takes into account only the latest changes",
			fields: fields{
				history: []healthapi.ClusterHealth{
					{
						Status:    healthapi.ClusterStatusDead,
						Timestamp: t1,
					},
					{
						Status:    healthapi.ClusterStatusDegraded,
						Timestamp: t2,
					},
					{
						Status:    healthapi.ClusterStatusDegraded,
						Timestamp: t3,
					},
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t3.Add(time.Hour),
					},
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t3.Add(time.Hour * 2),
					},
					{
						Status:    healthapi.ClusterStatusAlive,
						Timestamp: t3.Add(time.Hour * 3),
					},
				},
				numStatusesInFullWindow: 3,
			},
			want: Verdict{
				IsFlapping: false,
				Status:     healthapi.ClusterStatusAlive,
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			f := NewFlapAwareHealthObserver(tt.fields.numStatusesInFullWindow)
			for _, v := range tt.fields.history {
				f.LearnHealth(v)
			}
			if got := f.Verdict(); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("Verdict() = %v, want %v", got, tt.want)
			}
		})
	}
}
