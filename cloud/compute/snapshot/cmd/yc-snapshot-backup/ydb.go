package main

import (
	"context"
	"database/sql"
	"time"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"

	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

const (
	backupStarted string = "started"
	backupFailed  string = "failed"
	backupDone    string = "done"

	backupLockTimeout = statusUpdateInterval * 5
)

var (
	errCompleted = xerrors.New("completed. ok.")
	getChunkStmt *sql.Stmt
)

func initStmt(ctx context.Context) {
	var err error
	getChunkStmt, err = ydbDB.PrepareContext(ctx, `
DECLARE $id AS String;

SELECT data FROM [`+dbRoot+`/blobs_on_hdd]
WHERE id=$id;
`)
	ctxlog.InfoErrorCtx(ctx, err, "prepare get chunk query")
}

func getAllSnapshots(ctx context.Context, tx *sql.Tx) ([]string, error) {
	ctx = ctxlog.WithLogger(ctx, ctxlog.G(ctx).Named("getAllSnapshots"))
	lastID := ""
	var res []string

	for {
		rows, err := tx.QueryContext(ctx, `
DECLARE $lastID AS String;

SELECT id FROM [`+dbRoot+`/snapshotsext]
WHERE
	id > $lastID AND
	state = 'ready'
ORDER BY id
`, sql.Named("lastID", ydb.StringValue([]byte(lastID))))
		ctxlog.DebugErrorCtx(ctx, err, "Got snapshots batch")
		if err != nil {
			return nil, err
		}

		hasRowReaded := false
		for rows.Next() {
			hasRowReaded = true
			var id string
			err = rows.Scan(&id)
			if err != nil {
				ctxlog.G(ctx).Error("Error scan snapshot id", zap.Error(err))
				_ = rows.Close()
				return nil, err
			}
			res = append(res, id)
			lastID = id
		}
		_ = rows.Close()

		if rows.Err() != nil {
			return nil, rows.Err()
		}

		if !hasRowReaded {
			break
		}
	}
	return res, nil
}

type backupState struct {
	SnapshotID  string
	Status      string
	Percent     float32
	Counter     int64
	LastUpdated time.Time
}

func backupStatesGetAll(ctx context.Context, tx *sql.Tx) (map[string]*backupState, error) {
	ctx = ctxlog.WithLogger(ctx, ctxlog.G(ctx).Named("backupStatesGetAll"))
	lastID := ""
	res := make(map[string]*backupState)
	for {
		rows, err := tx.Query(`
DECLARE $lastID AS String;

SELECT snapshot_id, status, status_counter, status_percent, last_updated
FROM [`+dbRoot+`/mds_backup]
WHERE snapshot_id > $lastID
ORDER BY snapshot_id
`, sql.Named("lastID", ydb.StringValue([]byte(lastID))))
		ctxlog.DebugErrorCtx(ctx, err, "Got backup states batch")
		if err != nil {
			return nil, err
		}

		hasRowReaded := false
		for rows.Next() {
			hasRowReaded = true
			var item backupState
			var lastUpdated uint32
			err = rows.Scan(&item.SnapshotID, &item.Status, &item.Counter, &item.Percent, &lastUpdated)
			item.LastUpdated = time.Unix(int64(lastUpdated), 0)
			if err != nil {
				ctxlog.G(ctx).Error("Scan backup state", zap.Error(err))
				_ = rows.Close()
				return nil, err
			}
			lastID = item.SnapshotID
			res[item.SnapshotID] = &item
		}
		_ = rows.Close()

		if rows.Err() != nil {
			ctxlog.G(ctx).Error("Next backup state", zap.Error(err))
			return nil, rows.Err()
		}
		if !hasRowReaded {
			break
		}
	}
	return res, nil
}

func backupStatePut(ctx context.Context, state backupState) error {
	return retryInTx(ctx, "backupStatePut", func(ctx context.Context, tx *sql.Tx) error {
		return backupStatePutTx(ctx, state, tx)
	})
}

func backupStatePutTx(ctx context.Context, state backupState, tx *sql.Tx) error {
	_, err := tx.ExecContext(ctx, `
DECLARE $snapshot_id AS String;
DECLARE $status AS Utf8;
DECLARE $status_percent AS Float;
DECLARE $status_counter AS Int64;
DECLARE $last_updated AS Datetime;

UPSERT INTO [`+dbRoot+`/mds_backup] (snapshot_id, status, status_percent, status_counter, last_updated)
VALUES($snapshot_id, $status, $status_percent, $status_counter, $last_updated)
`,
		sql.Named("snapshot_id", ydb.StringValue([]byte(state.SnapshotID))),
		sql.Named("status", ydb.UTF8Value(string(state.Status))),
		sql.Named("status_counter", ydb.Int64Value(state.Counter)),
		sql.Named("status_percent", ydb.FloatValue(state.Percent)),
		sql.Named("last_updated", ydb.DatetimeValue(uint32(state.LastUpdated.Unix()))),
	)
	return err
}

func backupStateGet(ctx context.Context, id string, tx *sql.Tx) (*backupState, error) {
	row := tx.QueryRowContext(ctx, `
DECLARE $id AS String;

SELECT snapshot_id, status, status_counter, status_percent, last_updated
FROM [`+dbRoot+`/mds_backup]
WHERE snapshot_id = $id
`, sql.Named("id", ydb.StringValue([]byte(id))))
	var state backupState
	err := row.Scan(&state.SnapshotID, &state.Status, &state.Counter, &state.Percent, &state.LastUpdated)
	if err != nil {
		ctxlog.G(ctx).Error("Can't scan backup status", zap.Error(err))
		return nil, err
	}
	return &state, nil
}

func backupStateLock(ctx context.Context, snapshotID string) error {
	return runInTx(ctx, "update status", func(ctx context.Context, tx *sql.Tx) error {
		row := tx.QueryRowContext(ctx, `
DECLARE $snapshot_id AS String;

SELECT status, last_updated
FROM [`+dbRoot+`/mds_backup]
WHERE snapshot_id = $snapshot_id
`,
			sql.Named("snapshot_id", ydb.StringValue([]byte(snapshotID))),
		)
		var status string
		var lastUpdated uint32
		var lastUpdatedTime time.Time
		err := row.Scan(&status, &lastUpdated)
		lastUpdatedTime = time.Unix(int64(lastUpdated), 0)
		if err != nil && err != sql.ErrNoRows {
			ctxlog.G(ctx).Error("Can't scan status while update", zap.Error(err))
			return err
		}

		if err != sql.ErrNoRows && status != backupFailed && status != backupStarted && time.Since(lastUpdatedTime) < backupLockTimeout {
			return xerrors.Errorf("backup locked: %v (%v)", status, lastUpdated)
		}

		var state = backupState{
			SnapshotID:  snapshotID,
			Status:      backupStarted,
			LastUpdated: time.Now(),
		}
		return backupStatePutTx(ctx, state, tx)
	})
}

func selectSnapshotForBackup(ctx context.Context) (string, error) {
	var id string
	var err error
	_ = misc.Retry(ctx, "select snapshot", func() error {
		var backupStates map[string]*backupState
		err = retryInTx(ctx, "get backup states for select snapshot", func(ctx context.Context, tx *sql.Tx) error {
			backupStates, err = backupStatesGetAll(ctx, tx)
			return err
		})

		if err != nil {
			return misc.ErrInternalRetry
		}

		var allSnapshots []string
		err = retryInTx(ctx, "get all snapshots for select backup snapshot", func(ctx context.Context, tx *sql.Tx) error {
			allSnapshots, err = getAllSnapshots(ctx, tx)
			return err
		})
		if err != nil {
			return misc.ErrInternalRetry
		}
		id, err = selectSnapshotForBackupAlg(ctx, allSnapshots, backupStates)
		return err
	})
	return id, err
}

func selectSnapshotForBackupAlg(ctx context.Context, all []string, backups map[string]*backupState) (string, error) {
	allID := make(map[string]struct{}, len(all))
	for _, id := range all {
		allID[id] = struct{}{}
	}

	for _, state := range backups {
		if state.Status == backupDone || state.Status == backupStarted && time.Since(state.LastUpdated) < backupLockTimeout {
			delete(allID, state.SnapshotID)
		}
	}
	if len(allID) == 0 {
		return "", errCompleted
	}

	for id := range allID {
		err := backupStateLock(ctx, id)
		if err != nil {
			ctxlog.G(ctx).Error("Can't lock snapshot", zap.String("snapshot_id", id), zap.Error(err))
			continue
		}
		return id, nil
	}
	return "", xerrors.New("can't lock any snapshot")
}

func selectChunksForSnapshot(ctx context.Context, snapshotID string) ([]string, error) {
	var ids []string
	var lastOffset int64 = -1
	var err error
	_ = misc.Retry(ctx, "get snapshot chunks", func() error {
		for {
			var rows *sql.Rows
			rows, err = ydbDB.QueryContext(ctx, `
DECLARE $snapshotid AS String;
DECLARE $lastOffset AS Int64;

SELECT chunkid, chunkoffset, snapshotid, tree FROM [`+dbRoot+`/snapshotchunks]
WHERE
	snapshotid=$snapshotid AND tree=$snapshotid AND
	chunkoffset > $lastOffset
ORDER BY tree,snapshotid,chunkoffset
`,
				sql.Named("snapshotid", ydb.StringValue([]byte(snapshotID))),
				sql.Named("lastOffset", ydb.Int64Value(lastOffset)),
			)
			if err != nil {
				ctxlog.G(ctx).Error("get chunks", zap.Error(err))
				if kikimrRetryable(err) {
					return misc.ErrInternalRetry
				} else {
					return err
				}
			}
			hasRowReaded := false
			for rows.Next() {
				hasRowReaded = true
				var id string
				var offset int64
				var _stubSnapshotID string
				err = rows.Scan(&id, &offset, &_stubSnapshotID, &_stubSnapshotID)
				if err != nil {
					ctxlog.G(ctx).Error("error scan snapshot chunk", zap.Error(err))
					_ = rows.Close()
					return err
				}
				ids = append(ids, id)
				lastOffset = offset
			}
			_ = rows.Close()
			err = rows.Err()
			if err != nil {
				ctxlog.G(ctx).Error("get next chunks fro snapshot", zap.Error(err))
				if kikimrRetryable(err) {
					return misc.ErrInternalRetry
				} else {
					return err
				}
			}
			if !hasRowReaded {
				break
			}
		}
		return nil
	})
	if err == nil {
		return ids, nil
	} else {
		return nil, err
	}
}

func getChunk(ctx context.Context, id string) ([]byte, error) {
	var res []byte
	var err error
	_ = misc.Retry(ctx, "read chunkd from ydb", func() error {
		row := getChunkStmt.QueryRowContext(ctx, sql.Named("id", ydb.StringValue([]byte(id))))
		err = row.Scan(&res)
		if err != nil {
			if kikimrRetryable(err) {
				return misc.ErrInternalRetry
			} else {
				return err
			}
		}
		return nil
	})
	if err == nil {
		return res, nil
	} else {
		return nil, err
	}
}
