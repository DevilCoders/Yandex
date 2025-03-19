package provider

import (
	"fmt"
	"reflect"
	"testing"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
)

type backup struct {
	id   string
	init metadb.BackupInitiator
}

func (b backup) ID() string {
	return b.id
}

func (b backup) Initiator() metadb.BackupInitiator {
	return b.init
}

func TestPrevAutoBackup(t *testing.T) {
	type args struct {
		sortedBackups []SplittableBackup
		backupID      string
	}
	tests := []struct {
		name string
		args args
		want int
	}{
		{
			args: args{
				sortedBackups: []SplittableBackup{},
				backupID:      "1",
			},
			want: -1,
		},
		{
			args: args{
				sortedBackups: []SplittableBackup{
					backup{"1", metadb.BackupInitiatorSchedule},
					backup{"2", metadb.BackupInitiatorSchedule},
					backup{"3", metadb.BackupInitiatorSchedule},
				},
				backupID: "1",
			},
			want: -1,
		},
		{
			args: args{
				sortedBackups: []SplittableBackup{
					backup{"1", metadb.BackupInitiatorSchedule},
					backup{"2", metadb.BackupInitiatorSchedule},
					backup{"3", metadb.BackupInitiatorSchedule},
				},
				backupID: "2",
			},
			want: 0,
		},
		{
			args: args{
				sortedBackups: []SplittableBackup{
					backup{"1", metadb.BackupInitiatorSchedule},
					backup{"2", metadb.BackupInitiatorUser},
					backup{"3", metadb.BackupInitiatorSchedule},
				},
				backupID: "3",
			},
			want: 0,
		},
		{
			args: args{
				sortedBackups: []SplittableBackup{
					backup{"1", metadb.BackupInitiatorSchedule},
					backup{"2", metadb.BackupInitiatorUser},
					backup{"3", metadb.BackupInitiatorSchedule},
				},
				backupID: "2",
			},
			want: 0,
		},
		{
			args: args{
				sortedBackups: []SplittableBackup{
					backup{"1", metadb.BackupInitiatorUser},
					backup{"2", metadb.BackupInitiatorUser},
					backup{"3", metadb.BackupInitiatorSchedule},
				},
				backupID: "2",
			},
			want: -1,
		},
	}

	for _, tt := range tests {
		t.Run(fmt.Sprintf("prev_for_%s_in:%v", tt.args.backupID, tt.args.sortedBackups), func(t *testing.T) {
			if got := PrevAutoBackup(tt.args.sortedBackups, tt.args.backupID); got != tt.want {
				t.Errorf("PrevAutoBackup() = %v, wantRange %v", got, tt.want)
			}
		})
	}
}

func TestNearestAutoBackupIndexes(t *testing.T) {
	type args struct {
		backups  []SplittableBackup
		backupID string
	}
	tests := []struct {
		name             string
		args             args
		wantReq          int
		wantIntermManual []int
		wantNextAuto     int
		wantErr          bool
	}{
		{
			args: args{
				backups: []SplittableBackup{
					backup{"1", metadb.BackupInitiatorUser},
				},
				backupID: "1",
			},
			wantReq:          0,
			wantIntermManual: nil,
			wantNextAuto:     -1,
			wantErr:          false,
		},
		{
			args: args{
				backups: []SplittableBackup{
					backup{"1", metadb.BackupInitiatorSchedule},
				},
				backupID: "1",
			},
			wantReq:          0,
			wantIntermManual: nil,
			wantNextAuto:     -1,
			wantErr:          false,
		},
		{
			args: args{
				backups: []SplittableBackup{
					backup{"0", metadb.BackupInitiatorUser},
					backup{"1", metadb.BackupInitiatorUser},
					backup{"2", metadb.BackupInitiatorSchedule},
					backup{"3", metadb.BackupInitiatorUser},
					backup{"4", metadb.BackupInitiatorUser},
				},
				backupID: "2",
			},
			wantReq:          2,
			wantIntermManual: nil,
			wantNextAuto:     -1,
			wantErr:          false,
		},
		{
			args: args{
				backups: []SplittableBackup{
					backup{"0", metadb.BackupInitiatorUser},
					backup{"1", metadb.BackupInitiatorSchedule},
					backup{"2", metadb.BackupInitiatorSchedule},
				},
				backupID: "1",
			},
			wantReq:          1,
			wantIntermManual: nil,
			wantNextAuto:     2,
			wantErr:          false,
		},
		{
			args: args{
				backups: []SplittableBackup{
					backup{"0", metadb.BackupInitiatorUser},
					backup{"1", metadb.BackupInitiatorSchedule},
					backup{"2", metadb.BackupInitiatorSchedule},
					backup{"3", metadb.BackupInitiatorUser},
					backup{"4", metadb.BackupInitiatorSchedule},
					backup{"5", metadb.BackupInitiatorSchedule},
				},
				backupID: "2",
			},
			wantReq:          2,
			wantIntermManual: []int{3},
			wantNextAuto:     4,
			wantErr:          false,
		},
		{
			args: args{
				backups: []SplittableBackup{
					backup{"0", metadb.BackupInitiatorUser},
					backup{"1", metadb.BackupInitiatorSchedule},
					backup{"2", metadb.BackupInitiatorSchedule},
					backup{"3", metadb.BackupInitiatorUser},
					backup{"4", metadb.BackupInitiatorSchedule},
					backup{"5", metadb.BackupInitiatorUser},
					backup{"5", metadb.BackupInitiatorSchedule},
				},
				backupID: "2",
			},
			wantReq:          2,
			wantIntermManual: []int{3},
			wantNextAuto:     4,
			wantErr:          false,
		},
	}
	for _, tt := range tests {
		t.Run(fmt.Sprintf("nearest_for_%s_in:%+v", tt.args.backupID, tt.args.backups), func(t *testing.T) {
			gotReqAuto, gotIntermManual, gotNextAuto, err := NextAutoBackupIndexes(tt.args.backups, tt.args.backupID)
			if (err != nil) != tt.wantErr {
				t.Errorf("NextAutoBackupIndexes() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if gotReqAuto != tt.wantReq {
				t.Errorf("NextAutoBackupIndexes() gotReqAuto = %v, wantRange %v", gotReqAuto, tt.wantReq)
			}
			if !reflect.DeepEqual(gotIntermManual, tt.wantIntermManual) {
				t.Errorf("NextAutoBackupIndexes() gotIntermManual = %v, wantRange %v", gotIntermManual, tt.wantIntermManual)
			}
			if gotNextAuto != tt.wantNextAuto {
				t.Errorf("NextAutoBackupIndexes() gotNextAuto = %v, wantRange %v", gotNextAuto, tt.wantNextAuto)
			}
		})
	}
}
