package invoicer

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"
)

func TestSplitRangeByHours(t *testing.T) {
	type args struct {
		r Range
	}
	tests := []struct {
		name string
		args args
		want []Range
	}{
		{
			name: "no_split",
			args: args{
				r: Range{
					FromTS:  time.Date(2022, 01, 01, 14, 00, 59, 0, time.UTC),
					UntilTS: time.Date(2022, 01, 01, 14, 50, 10, 0, time.UTC),
					Payload: "data",
				},
			},
			want: []Range{
				{
					FromTS:  time.Date(2022, 01, 01, 14, 00, 59, 0, time.UTC),
					UntilTS: time.Date(2022, 01, 01, 14, 50, 10, 0, time.UTC),
					Payload: "data",
				},
			},
		},
		{
			name: "split",
			args: args{
				r: Range{
					FromTS:  time.Date(2022, 01, 01, 14, 00, 59, 0, time.UTC),
					UntilTS: time.Date(2022, 01, 01, 15, 50, 10, 0, time.UTC),
					Payload: "data",
				},
			},
			want: []Range{
				{
					FromTS:  time.Date(2022, 01, 01, 14, 00, 59, 0, time.UTC),
					UntilTS: time.Date(2022, 01, 01, 15, 00, 00, 0, time.UTC),
					Payload: "data",
				},
				{
					FromTS:  time.Date(2022, 01, 01, 15, 00, 00, 0, time.UTC),
					UntilTS: time.Date(2022, 01, 01, 15, 50, 10, 0, time.UTC),
					Payload: "data",
				},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.want, SplitRangeByHours(tt.args.r))
		})
	}
}

func TestChunkInvoices(t *testing.T) {
	type args struct {
		invoices  []Invoice
		chunkSize int
	}
	tests := []struct {
		name string
		args args
		want [][]Invoice
	}{
		{
			name: "empty",
			args: args{
				invoices:  []Invoice{},
				chunkSize: 5,
			},
			want: nil,
		},
		{
			name: "one_inv",
			args: args{
				invoices:  []Invoice{{BillType: "1"}},
				chunkSize: 50,
			},
			want: [][]Invoice{
				{{BillType: "1"}},
			},
		},
		{
			name: "no_split",
			args: args{
				invoices:  []Invoice{{BillType: "1"}, {BillType: "2"}, {BillType: "3"}, {BillType: "4"}},
				chunkSize: 5,
			},
			want: [][]Invoice{
				{{BillType: "1"}, {BillType: "2"}, {BillType: "3"}, {BillType: "4"}},
			},
		},
		{
			name: "split_two",
			args: args{
				invoices:  []Invoice{{BillType: "1"}, {BillType: "2"}, {BillType: "3"}, {BillType: "4"}},
				chunkSize: 2,
			},
			want: [][]Invoice{
				{{BillType: "1"}, {BillType: "2"}},
				{{BillType: "3"}, {BillType: "4"}},
			},
		},
		{
			name: "split_four",
			args: args{
				invoices:  []Invoice{{BillType: "1"}, {BillType: "2"}, {BillType: "3"}, {BillType: "4"}, {BillType: "5"}, {BillType: "6"}, {BillType: "7"}},
				chunkSize: 2,
			},
			want: [][]Invoice{
				{{BillType: "1"}, {BillType: "2"}},
				{{BillType: "3"}, {BillType: "4"}},
				{{BillType: "5"}, {BillType: "6"}},
				{{BillType: "7"}},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.want, ChunkInvoices(tt.args.invoices, tt.args.chunkSize))
		})
	}
}
