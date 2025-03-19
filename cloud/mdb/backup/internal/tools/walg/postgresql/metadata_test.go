package postgresql

import (
	"reflect"
	"testing"
)

func TestCommonPrefixesForWals(t *testing.T) {
	type args struct {
		timelines []uint32
		beginLSN  uint64
		endLSN    uint64
	}
	tests := []struct {
		name string
		args args
		want []string
	}{
		{
			args: args{
				timelines: []uint32{1, 2},
				beginLSN:  1 * WalSegmentSize,
				endLSN:    2 * WalSegmentSize,
			},
			want: []string{"00000001000000000000000", "00000002000000000000000"},
		},
		{
			args: args{
				timelines: []uint32{1},
				beginLSN:  11110 * WalSegmentSize,
				endLSN:    11115 * WalSegmentSize,
			},
			want: []string{"000000010000002B0000006"},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := CommonPrefixesFromTimelineLsn(tt.args.timelines, tt.args.beginLSN, tt.args.endLSN); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("CommonPrefixesFromTimelineLsn() = %v, want %v", got, tt.want)
			}
		})
	}
}
