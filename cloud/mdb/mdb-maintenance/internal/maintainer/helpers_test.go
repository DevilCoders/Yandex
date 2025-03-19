package maintainer

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
)

func Test_toExclude(t *testing.T) {
	type args struct {
		src       []models.Cluster
		toExclude []models.Cluster
	}
	tests := []struct {
		name string
		args args
		want []models.Cluster
	}{
		{
			name: "src empty",
			args: args{
				src:       []models.Cluster{},
				toExclude: []models.Cluster{{ID: "1"}},
			},
			want: []models.Cluster{},
		},
		{
			name: "exclusion empty",
			args: args{
				src:       []models.Cluster{{ID: "1"}},
				toExclude: []models.Cluster{},
			},
			want: []models.Cluster{{ID: "1"}},
		},
		{
			name: "all excluded",
			args: args{
				src:       []models.Cluster{{ID: "1"}},
				toExclude: []models.Cluster{{ID: "1"}},
			},
			want: []models.Cluster{},
		},
		{
			name: "subset excluded",
			args: args{
				src:       []models.Cluster{{ID: "1"}, {ID: "2"}},
				toExclude: []models.Cluster{{ID: "2"}},
			},
			want: []models.Cluster{{ID: "1"}},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got := excludeClusters(tt.args.src, tt.args.toExclude)
			if len(got) == 0 && len(tt.want) == 0 {
				return
			}
			assert.EqualValues(t, tt.want, got, "excludeClusters() = %v, want %v", got, tt.want)
		})
	}
}
