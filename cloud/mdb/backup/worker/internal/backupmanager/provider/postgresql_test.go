package provider

import (
	"fmt"
	"reflect"
	"testing"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/internal/tools/walg/postgresql"
)

func TestFilterWalsByRange(t *testing.T) {
	type args struct {
		wals    []postgresql.WalSegment
		r       Range
		exclude []Range
	}
	tests := []struct {
		args args
		want []postgresql.WalSegment
	}{
		{
			args: args{
				wals: []postgresql.WalSegment{{StartLSN: 0}, {StartLSN: 1}, {StartLSN: 2}, {StartLSN: 3}},
				r: Range{
					Begin: Point{Timeline: 0, LSN: 1},
					End:   Point{Timeline: 0, LSN: 4},
				},
				exclude: nil,
			},
			want: []postgresql.WalSegment{{StartLSN: 1}, {StartLSN: 2}, {StartLSN: 3}},
		},
		{
			args: args{
				wals: []postgresql.WalSegment{{StartLSN: 0}, {StartLSN: 1}, {StartLSN: 2}, {StartLSN: 3}},
				r: Range{
					Begin: Point{Timeline: 0, LSN: 1},
					End:   Point{Timeline: 0, LSN: 4},
				},
				exclude: []Range{
					{Begin: Point{Timeline: 0, LSN: 0}, End: Point{Timeline: 0, LSN: 1}},
					{Begin: Point{Timeline: 0, LSN: 3}, End: Point{Timeline: 0, LSN: 4}},
				},
			},
			want: []postgresql.WalSegment{{StartLSN: 2}},
		},
		{
			args: args{
				wals: []postgresql.WalSegment{{StartLSN: 0}, {StartLSN: 1}, {StartLSN: 2}, {StartLSN: 3}},
				r: Range{
					Begin: Point{Timeline: 0, LSN: 0},
					End:   Point{Timeline: 0, LSN: 1},
				},
				exclude: nil,
			},
			want: []postgresql.WalSegment{{StartLSN: 0}, {StartLSN: 1}},
		},
	}
	for _, tt := range tests {
		t.Run(fmt.Sprintf("%+v_in_range_%+v_exclude_%+v", tt.args.wals, tt.args.wals, tt.args.exclude), func(t *testing.T) {
			if got := FilterWalsByRange(tt.args.wals, tt.args.r, tt.args.exclude); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("FilterWalsByRange() = %v, wantRange %v", got, tt.want)
			}
		})
	}
}

func TestIsLSNInRanges(t *testing.T) {
	type args struct {
		lsn    uint64
		ranges []Range
	}
	tests := []struct {
		args args
		want bool
	}{
		{
			args: args{
				lsn:    0,
				ranges: []Range{},
			},
			want: false,
		},
		{
			args: args{
				lsn:    0,
				ranges: nil,
			},
			want: false,
		},
		{
			args: args{
				lsn: 0,
				ranges: []Range{{
					Begin: Point{LSN: 1},
					End:   Point{LSN: 3},
				}},
			},
			want: false,
		},
		{
			args: args{
				lsn: 4,
				ranges: []Range{{
					Begin: Point{LSN: 1},
					End:   Point{LSN: 3},
				}},
			},
			want: false,
		},
		{
			args: args{
				lsn: 2,
				ranges: []Range{{
					Begin: Point{LSN: 1},
					End:   Point{LSN: 3},
				}},
			},
			want: true,
		},
		{
			args: args{
				lsn: 4,
				ranges: []Range{
					{
						Begin: Point{LSN: 1},
						End:   Point{LSN: 3},
					},
					{
						Begin: Point{LSN: 5},
						End:   Point{LSN: 6},
					},
				},
			},
			want: false,
		},
		{
			args: args{
				lsn: 7,
				ranges: []Range{
					{
						Begin: Point{LSN: 1},
						End:   Point{LSN: 3},
					},
					{
						Begin: Point{LSN: 5},
						End:   Point{LSN: 6},
					},
				},
			},
			want: false,
		},
		{
			args: args{
				lsn: 6,
				ranges: []Range{
					{
						Begin: Point{LSN: 1},
						End:   Point{LSN: 3},
					},
					{
						Begin: Point{LSN: 5},
						End:   Point{LSN: 6},
					},
				},
			},
			want: true,
		},
	}
	for _, tt := range tests {
		t.Run(fmt.Sprintf("lsn_%d_ranges_%+v", tt.args.lsn, tt.args.ranges), func(t *testing.T) {
			if got := IsLSNInRanges(tt.args.lsn, tt.args.ranges); got != tt.want {
				t.Errorf("IsLSNInRanges() = %v, wantRange %v", got, tt.want)
			}
		})
	}
}

func TestRange_In(t *testing.T) {
	type fields struct {
		Begin Point
		End   Point
	}
	type args struct {
		lsn uint64
	}
	tests := []struct {
		fields fields
		args   args
		want   bool
	}{
		{
			fields: fields{Begin: Point{Timeline: 0, LSN: 5}, End: Point{Timeline: 0, LSN: 10}},
			args:   args{lsn: 5},
			want:   true,
		},
		{
			fields: fields{Begin: Point{Timeline: 0, LSN: 5}, End: Point{Timeline: 0, LSN: 10}},
			args:   args{lsn: 7},
			want:   true,
		},
		{
			fields: fields{Begin: Point{Timeline: 0, LSN: 5}, End: Point{Timeline: 0, LSN: 10}},
			args:   args{lsn: 10},
			want:   true,
		},
		{
			fields: fields{Begin: Point{Timeline: 0, LSN: 5}, End: Point{Timeline: 0, LSN: 10}},
			args:   args{lsn: 3},
			want:   false,
		},
		{
			fields: fields{Begin: Point{Timeline: 0, LSN: 5}, End: Point{Timeline: 0, LSN: 10}},
			args:   args{lsn: 11},
			want:   false,
		},
	}
	for _, tt := range tests {
		t.Run(fmt.Sprintf("lsn_%d_in_range_%+v", tt.args.lsn, tt.fields), func(t *testing.T) {
			r := Range{
				Begin: tt.fields.Begin,
				End:   tt.fields.End,
			}
			if got := r.In(tt.args.lsn); got != tt.want {
				t.Errorf("In() = %v, wantRange %v", got, tt.want)
			}
		})
	}
}

func TestRangeForAutoBackup(t *testing.T) {
	type args struct {
		sortedBackups []FullPostgreSQLBackup
		backupID      string
	}
	tests := []struct {
		name        string
		args        args
		wantRange   Range
		wantExclude []Range
		wantErr     bool
	}{
		{
			name: "",
			args: args{
				sortedBackups: []FullPostgreSQLBackup{
					{
						Backup: metadb.Backup{BackupID: "1"},
						ParsedMetadata: FullPostgreSQLMetadata{
							PostgreSQLMetadata: postgresql.PostgreSQLMetadata{BackupName: "base_000000010000000000000000"},
							ParsedDtoMeta:      postgresql.ExtendedMetadataDto{StartLsn: 10, FinishLsn: 20},
						},
					},
				},
				backupID: "1",
			},
			wantRange:   Range{Begin: Point{Timeline: 1, LSN: 10}, End: Point{Timeline: 1, LSN: 20}},
			wantExclude: nil,
			wantErr:     false,
		},
		{
			name: "",
			args: args{
				sortedBackups: []FullPostgreSQLBackup{
					{
						Backup: metadb.Backup{BackupID: "1", Initiator: metadb.BackupInitiatorSchedule},
						ParsedMetadata: FullPostgreSQLMetadata{
							PostgreSQLMetadata: postgresql.PostgreSQLMetadata{BackupName: "base_000000010000000000000000"},
							ParsedDtoMeta:      postgresql.ExtendedMetadataDto{StartLsn: 10, FinishLsn: 20},
						},
					},
					{
						Backup: metadb.Backup{BackupID: "2", Initiator: metadb.BackupInitiatorUser},
						ParsedMetadata: FullPostgreSQLMetadata{
							PostgreSQLMetadata: postgresql.PostgreSQLMetadata{BackupName: "base_000000010000000000000000"},
							ParsedDtoMeta:      postgresql.ExtendedMetadataDto{StartLsn: 20, FinishLsn: 30},
						},
					},
				},
				backupID: "1",
			},
			wantRange:   Range{Begin: Point{Timeline: 1, LSN: 10}, End: Point{Timeline: 1, LSN: 20}},
			wantExclude: nil,
			wantErr:     false,
		},
		{
			name: "",
			args: args{
				sortedBackups: []FullPostgreSQLBackup{
					{
						Backup: metadb.Backup{BackupID: "1", Initiator: metadb.BackupInitiatorSchedule},
						ParsedMetadata: FullPostgreSQLMetadata{
							PostgreSQLMetadata: postgresql.PostgreSQLMetadata{BackupName: "base_000000010000000000000000"},
							ParsedDtoMeta:      postgresql.ExtendedMetadataDto{StartLsn: 10, FinishLsn: 20},
						},
					},
					{
						Backup: metadb.Backup{BackupID: "2", Initiator: metadb.BackupInitiatorSchedule},
						ParsedMetadata: FullPostgreSQLMetadata{
							PostgreSQLMetadata: postgresql.PostgreSQLMetadata{BackupName: "base_000000020000000000000000"},
							ParsedDtoMeta:      postgresql.ExtendedMetadataDto{StartLsn: 30, FinishLsn: 40},
						},
					},
				},
				backupID: "1",
			},
			wantRange:   Range{Begin: Point{Timeline: 1, LSN: 10}, End: Point{Timeline: 2, LSN: 30}},
			wantExclude: []Range{},
			wantErr:     false,
		},
		{
			name: "",
			args: args{
				sortedBackups: []FullPostgreSQLBackup{
					{
						Backup: metadb.Backup{BackupID: "1", Initiator: metadb.BackupInitiatorSchedule},
						ParsedMetadata: FullPostgreSQLMetadata{
							PostgreSQLMetadata: postgresql.PostgreSQLMetadata{BackupName: "base_000000010000000000000000"},
							ParsedDtoMeta:      postgresql.ExtendedMetadataDto{StartLsn: 10, FinishLsn: 20},
						},
					},
					{
						Backup: metadb.Backup{BackupID: "2", Initiator: metadb.BackupInitiatorUser},
						ParsedMetadata: FullPostgreSQLMetadata{
							PostgreSQLMetadata: postgresql.PostgreSQLMetadata{BackupName: "base_000000020000000000000000"},
							ParsedDtoMeta:      postgresql.ExtendedMetadataDto{StartLsn: 30, FinishLsn: 40},
						},
					},
					{
						Backup: metadb.Backup{BackupID: "3", Initiator: metadb.BackupInitiatorSchedule},
						ParsedMetadata: FullPostgreSQLMetadata{
							PostgreSQLMetadata: postgresql.PostgreSQLMetadata{BackupName: "base_000000020000000000000000_D_000000010000000000000000"},
							ParsedDtoMeta:      postgresql.ExtendedMetadataDto{StartLsn: 50, FinishLsn: 60},
						},
					},
				},
				backupID: "1",
			},
			wantRange:   Range{Begin: Point{Timeline: 1, LSN: 10}, End: Point{Timeline: 2, LSN: 50}},
			wantExclude: []Range{{Begin: Point{Timeline: 2, LSN: 30}, End: Point{Timeline: 2, LSN: 40}}},
			wantErr:     false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			backupRange, excludeRanges, err := RangeForAutoBackup(tt.args.sortedBackups, tt.args.backupID)
			if (err != nil) != tt.wantErr {
				t.Errorf("RangeForAutoBackup() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if !reflect.DeepEqual(backupRange, tt.wantRange) {
				t.Errorf("RangeForAutoBackup() backupRange = %v, wantRange %v", backupRange, tt.wantRange)
			}
			if !reflect.DeepEqual(excludeRanges, tt.wantExclude) {
				t.Errorf("RangeForAutoBackup() excludeRanges = %v, wantRange %v", excludeRanges, tt.wantExclude)
			}
		})
	}
}
