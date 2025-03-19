package main

import (
	"context"
	"encoding/json"
	"fmt"
	"strings"
	"time"

	"github.com/golang/protobuf/jsonpb"

	private_protos "a.yandex-team.ru/cloud/blockstore/private/api/protos"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

const maxQueryTimeout = time.Second
const minQueryTimeout = time.Second * 10

////////////////////////////////////////////////////////////////////////////////

type blockCountExtraInfo struct {
	InitialMixedBlocks  uint64
	InitialMergedBlocks uint64
	InitialBlobs        uint64

	FinalMixedBlocks  uint64
	FinalMergedBlocks uint64
	FinalBlobs        uint64
}

////////////////////////////////////////////////////////////////////////////////

func executeRebuildMetadata(
	ctx context.Context,
	request *private_protos.TRebuildMetadataRequest) error {

	response := &private_protos.TRebuildMetadataResponse{}

	input, err := new(jsonpb.Marshaler).MarshalToString(request)
	if err != nil {
		return err
	}

	client := getClient(ctx)

	output, err := client.ExecuteAction(ctx, "RebuildMetadata", []byte(input))
	if err != nil {
		return err
	}

	err = new(jsonpb.Unmarshaler).Unmarshal(
		strings.NewReader(string(output)),
		response,
	)

	return err
}

func pollRebuildStatus(
	ctx context.Context,
	volume string) (*private_protos.TMetadataRebuildProgress, error) {

	request := &private_protos.TGetRebuildMetadataStatusRequest{
		DiskId: volume,
	}

	response := &private_protos.TGetRebuildMetadataStatusResponse{}

	input, err := new(jsonpb.Marshaler).MarshalToString(request)
	if err != nil {
		return nil, err
	}

	client := getClient(ctx)

	output, err := client.ExecuteAction(ctx, "GetRebuildMetadataStatus", []byte(input))
	if err != nil {
		return nil, err
	}

	err = new(jsonpb.Unmarshaler).Unmarshal(
		strings.NewReader(string(output)),
		response,
	)
	if err != nil {
		return nil, err
	}

	return response.Progress, nil
}

////////////////////////////////////////////////////////////////////////////////

type rebuildMetadataWorkerIface interface {
	Execute(ctx context.Context) error
	UpdateProgress(ctx context.Context) (*private_protos.TMetadataRebuildProgress, error)
	Success(ctx context.Context, start time.Time) (*rebuildStats, error)
	Fail(ctx context.Context, start time.Time) *rebuildStats
}

////////////////////////////////////////////////////////////////////////////////

type rebuildMetadataBlockCountWorker struct {
	stats     *rebuildStats
	extraInfo blockCountExtraInfo
	diskID    string
	batchSize uint32
}

func (r *rebuildMetadataBlockCountWorker) Execute(ctx context.Context) error {
	client := getClient(ctx)
	log := getLog(ctx)

	_, stats, err := client.StatVolume(ctx, r.diskID, 0)
	if err != nil {
		return err
	}

	log.logDebug(
		ctx,
		"Disk %v stats: mixed blobs %v merged blobs %v",
		r.diskID,
		stats.MixedBlobsCount,
		stats.MergedBlobsCount)
	r.extraInfo.InitialBlobs = stats.MixedBlobsCount + stats.MergedBlobsCount
	r.extraInfo.InitialMixedBlocks = stats.MixedBlocksCount
	r.extraInfo.InitialMergedBlocks = stats.MergedBlocksCount

	request := &private_protos.TRebuildMetadataRequest{
		DiskId:       r.diskID,
		MetadataType: private_protos.ERebuildMetadataType_BLOCK_COUNT,
		BatchSize:    r.batchSize,
	}

	return executeRebuildMetadata(ctx, request)
}

func (r *rebuildMetadataBlockCountWorker) UpdateProgress(
	ctx context.Context) (*private_protos.TMetadataRebuildProgress, error) {

	progress, err := pollRebuildStatus(ctx, r.diskID)
	if err == nil {
		r.stats.processed = progress.Processed
		r.stats.total = progress.Total
	}
	return progress, err
}

func (r *rebuildMetadataBlockCountWorker) Success(
	ctx context.Context,
	start time.Time) (*rebuildStats, error) {

	client := getClient(ctx)

	_, stats, err := client.StatVolume(ctx, r.diskID, 0)
	if err != nil {
		return nil, err
	}
	r.stats.operationDuration = time.Since(start)
	r.extraInfo.FinalMixedBlocks = stats.MixedBlocksCount
	r.extraInfo.FinalMergedBlocks = stats.MergedBlocksCount
	r.extraInfo.FinalBlobs = stats.MixedBlobsCount + stats.MergedBlobsCount

	o, err := json.Marshal(r.extraInfo)
	if err != nil {
		return r.stats, err
	}
	r.stats.extra = string(o)

	return r.stats, nil
}

func (r *rebuildMetadataBlockCountWorker) Fail(
	ctx context.Context,
	start time.Time) *rebuildStats {

	r.stats.operationDuration = time.Since(start)
	return r.stats
}

////////////////////////////////////////////////////////////////////////////////

type rebuildMetadataUsedBlocksWorker struct {
	stats     *rebuildStats
	diskID    string
	batchSize uint32
}

func (r *rebuildMetadataUsedBlocksWorker) Execute(ctx context.Context) error {
	request := &private_protos.TRebuildMetadataRequest{
		DiskId:       r.diskID,
		MetadataType: private_protos.ERebuildMetadataType_USED_BLOCKS,
		BatchSize:    r.batchSize,
	}

	return executeRebuildMetadata(ctx, request)
}

func (r *rebuildMetadataUsedBlocksWorker) UpdateProgress(
	ctx context.Context) (*private_protos.TMetadataRebuildProgress, error) {

	progress, err := pollRebuildStatus(ctx, r.diskID)
	if err == nil {
		r.stats.processed = progress.Processed
		r.stats.total = progress.Total
	}
	return progress, err
}

func (r *rebuildMetadataUsedBlocksWorker) Success(
	ctx context.Context, start time.Time) (*rebuildStats, error) {

	r.stats.operationDuration = time.Since(start)
	return r.stats, nil
}

func (r *rebuildMetadataUsedBlocksWorker) Fail(
	ctx context.Context,
	start time.Time) *rebuildStats {

	r.stats.operationDuration = time.Since(start)
	return r.stats
}

////////////////////////////////////////////////////////////////////////////////

func worker(
	ctx context.Context,
	log *rebuildMetadataLog,
	diskID string,
	opts *options,
	ch chan<- opResponse) {

	var work rebuildMetadataWorkerIface
	requestStats := &rebuildStats{
		diskID: diskID,
	}

	ctx = withLog(ctx, log)

	client, err := createNbsClient(ctx, opts)
	if err != nil {
		ch <- opResponse{stats: requestStats, result: err}
		return
	}
	defer client.Close()

	ctx = withNbsClient(ctx, *client)

	if opts.rebuildType == private_protos.ERebuildMetadataType_BLOCK_COUNT {
		work = &rebuildMetadataBlockCountWorker{
			stats:     requestStats,
			diskID:    diskID,
			batchSize: opts.batchSize,
		}
	} else {
		work = &rebuildMetadataUsedBlocksWorker{
			stats:     requestStats,
			diskID:    diskID,
			batchSize: opts.batchSize,
		}
	}

	start := time.Now()

	log.logInfo(ctx, "Starting rebuild metadata for disk %v", diskID)

	err = work.Execute(ctx)
	if err != nil {
		ch <- opResponse{stats: work.Fail(ctx, start), result: err}
		return
	}

	delay := minQueryTimeout

	for {
		time.Sleep(time.Duration(delay))

		progress, err := work.UpdateProgress(ctx)
		if err != nil {
			if clientError, ok := err.(*nbs.ClientError); ok {
				if clientError.Code == nbs.E_NOT_FOUND {
					err = fmt.Errorf("tablet for disk %s seems to be restarted", diskID)
				}
			}
			ch <- opResponse{stats: work.Fail(ctx, start), result: err}
			return
		}

		if progress.Processed != 0 {
			done := float64(progress.Processed) / float64(progress.Total)
			timePassed := time.Since(start)
			delay = time.Duration(float64(timePassed)/done - float64(timePassed))

			log.logDebug(ctx, "Disk %v is expected to complete in %v", diskID, delay)

			if delay > maxQueryTimeout {
				delay = maxQueryTimeout
			}
			if delay < minQueryTimeout {
				delay = minQueryTimeout
			}

			log.logInfo(ctx, "Will query disk %v progress in %v", diskID, delay)
		}

		if progress.IsCompleted {
			stats, err := work.Success(ctx, start)
			ch <- opResponse{stats: stats, result: err}
			return
		} else {
			log.logInfo(ctx, "Disk %v progress: %v of %v", diskID, progress.Processed, progress.Total)
		}
	}
}
