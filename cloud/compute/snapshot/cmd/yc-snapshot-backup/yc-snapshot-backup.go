package main

import (
	"context"
	"fmt"
	"math/rand"
	"os"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

const (
	statusUpdateInterval = time.Minute
)

var (
	keyID            = os.Getenv("AWS_ACCESS_KEY_ID")
	keySecret        = os.Getenv("AWS_SECRET_ACCESS_KEY")
	prefix           = os.Getenv("STORE_PREFIX")
	uploaderCount, _ = strconv.Atoi(os.Getenv("UPLOADERS"))
	bucket           = "cloud-snapshot-backup"
	encryptKeyHex    = os.Getenv("ENCRYPT_KEY")
	encryptKey       []byte
)

func main() {
	rand.Seed(time.Now().UnixNano())

	logger, _ := zap.NewDevelopment()
	ctx := ctxlog.WithLogger(context.Background(), logger)
	if prefix == "" {
		logger.Fatal("need STORE_PREFIX")
	}
	if uploaderCount == 0 {
		logger.Fatal("need UPLOADERS")
	}

	_, err := fmt.Fscanf(strings.NewReader(encryptKeyHex), "%x", &encryptKey)
	if err != nil {
		logger.Fatal("need hex ENCRYPT_KEY")
	}

	initStmt(ctx)

	for {
		snapshotID, err := selectSnapshotForBackup(ctx)
		if err == errCompleted {
			break
		}
		if err != nil {
			logger.Fatal("Can't select next snapshot", zap.Error(err))
		}

		err = snapshotBackup(ctx, snapshotID)
		ctxlog.DebugErrorCtx(ctx, err, "Backup snapshot completed", zap.String("snapshot_id", snapshotID))
	}
}

func snapshotBackup(ctx context.Context, snapshotID string) error {
	ctx, ctxCancel := context.WithCancel(ctx)
	defer ctxCancel()
	ctx = ctxlog.WithLogger(ctx, ctxlog.G(ctx).With(zap.String("snapshot_id", snapshotID)))

	var resErrCnt int64
	var resErr error

	chunkIDs, err := selectChunksForSnapshot(ctx, snapshotID)
	ctxlog.DebugErrorCtx(ctx, err, "Got chunks for snapshot")
	if err != nil {
		return err
	}

	chunkTask := make(chan string)

	var wg sync.WaitGroup
	wg.Add(uploaderCount)
	for i := 0; i < uploaderCount; i++ {
		go func() {
			defer wg.Done()

			for chunkID := range chunkTask {
				if ctx.Err() != nil {
					return
				}

				chunkErr := backupChunk(ctx, snapshotID, chunkID)
				if chunkErr == nil {
					_, _ = fmt.Fprint(os.Stderr, ".")
				} else {
					resErrCnt := atomic.AddInt64(&resErrCnt, 1)
					if resErrCnt == 1 {
						resErr = chunkErr
					}
					ctxCancel()
				}
			}
		}()
	}

	state := backupState{
		SnapshotID:  snapshotID,
		Status:      backupStarted,
		LastUpdated: time.Now(),
	}
	var lastIndex int
	for index, chunkID := range chunkIDs {
		if ctx.Err() != nil {
			break
		}
		state.Counter = int64(index + 1)
		chunkTask <- chunkID
		if time.Since(state.LastUpdated) > statusUpdateInterval && index < len(chunkIDs)-1 {
			state.Percent = float32(state.Counter) / float32(len(chunkIDs))
			state.LastUpdated = time.Now()
			go func(state backupState) {
				ctxState, ctxStateCancel := context.WithTimeout(ctx, time.Second)
				defer ctxStateCancel()

				stateErr := backupStatePut(ctxState, state)
				if stateErr != nil {
					ctxlog.G(ctx).Error("save backup state", zap.Error(err))
				}
			}(state)
		}
	}
	close(chunkTask)
	wg.Wait()

	state.Percent = float32(lastIndex+1) / float32(len(chunkIDs))

	state.LastUpdated = time.Now()
	if resErr == nil {
		state.Status = backupDone
	} else {
		state.Status = backupFailed
	}
	err = backupStatePut(ctx, state)
	ctxlog.InfoErrorCtx(ctx, err, "Save snapshot state")
	fmt.Printf("Snapshot: '%v' %v (%v)\n", snapshotID, state.Status, time.Now())
	return resErr
}

func backupChunk(ctx context.Context, snapshotID, chunkID string) error {
	data, err := getChunk(ctx, chunkID)
	if err != nil {
		ctxlog.G(ctx).Error("can't get chunk", zap.String("dhunk_id", chunkID), zap.Error(err))
	}
	data = encrypt(ctx, encryptKey, data)
	key := prefix + "/" + snapshotID + "/" + chunkID
	err = s3Put(bucket, key, data)
	if err != nil {
		ctxlog.G(ctx).Error("can't store chunk", zap.String("chunk_id", chunkID), zap.Error(err))
	}
	return err
}
