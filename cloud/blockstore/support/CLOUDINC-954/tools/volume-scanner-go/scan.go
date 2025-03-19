package main

import (
	"context"

	protos "a.yandex-team.ru/cloud/blockstore/private/api/protos"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

type BlockRange struct {
	StartIndex  uint32
	BlocksCount uint32
}

func scanRangeAsync(
	ctx context.Context,
	client *nbs.Client,
	diskId string,
	startIndex uint32,
	blocksCount uint32,
	out chan BlockRange,
	outErr chan error) {

	request := &protos.TDescribeBlocksRequest{
		DiskId:      diskId,
		StartIndex:  startIndex,
		BlocksCount: blocksCount,
	}

	response, err := DescribeBlocks(ctx, client, request)
	if err != nil {
		outErr <- err
		return
	}

	for _, p := range response.BlobPieces {
		request := &protos.TCheckBlobRequest{
			BlobId:    p.BlobId,
			BSGroupId: p.BSGroupId,
		}

		response, err := CheckBlob(ctx, client, request)
		if err != nil {
			outErr <- err
			return
		}

		if response.Status != "OK" {
			for _, r := range p.Ranges {
				out <- BlockRange{r.BlockIndex, r.BlocksCount}
			}
		}
	}

	outErr <- nil
}

func ScanRange(
	ctx context.Context,
	client *nbs.Client,
	diskId string,
	blocksCount uint32,
	batch uint32,
) ([]BlockRange, error) {

	out := make(chan BlockRange)
	outErr := make(chan error)

	var badRanges []BlockRange
	pending := 0

	var i uint32
	for i < blocksCount {
		count := blocksCount - i
		if count > batch {
			count = batch
		}

		go scanRangeAsync(ctx, client, diskId, i, count, out, outErr)
		pending++
		i = i + count
	}

	for {
		select {
		case r := <-out:
			badRanges = append(badRanges, r)
		case err := <-outErr:
			if err != nil {
				return nil, err
			}
			pending--
			if pending == 0 {
				return badRanges, nil
			}
		}
	}
}
