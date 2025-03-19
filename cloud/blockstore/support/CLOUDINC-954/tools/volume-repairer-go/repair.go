package main

import (
	"bufio"
	"context"
	"fmt"
	"os"
	"sort"
	"strconv"
	"strings"
	"time"

	protos "a.yandex-team.ru/cloud/blockstore/private/api/protos"
	pprotos "a.yandex-team.ru/cloud/blockstore/public/api/protos"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

type BlockRange struct {
	StartIndex  uint32
	BlocksCount uint32
}

type RangeStatus uint32

const (
	rangeStatusScanned  RangeStatus = iota
	rangeStatusBad      RangeStatus = iota
	rangeStatusRepaired RangeStatus = iota
)

func getSocketPath(diskId string) string {
	return fmt.Sprintf("/var/tmp/repair-%s.socket", diskId)
}

func saveRange(blockRange BlockRange, status RangeStatus, fileName string) {

	f, err := os.OpenFile(fileName, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)

	if err != nil {
		fmt.Printf("Failed opening file %s\n", err)
		return
	}

	defer f.Close()

	str := fmt.Sprintf("%v %v %v", blockRange.StartIndex, blockRange.BlocksCount, status)

	f.WriteString(str + "\n")
}

func restoreRanges(
	blocksCount uint32,
	batch uint32,
	fileName string,
) ([]BlockRange, []BlockRange, error) {

	scannedRanges := make(map[int]RangeStatus)
	badRangesMap := make(map[int]BlockRange)

	file, err := os.Open(fileName)
	if err != nil {
		return nil, nil, err
	}

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		line := scanner.Text()
		fields := strings.Fields(line)

		if len(fields) < 3 {
			return nil, nil, fmt.Errorf("fail to read range line: %s", line)
		}

		start, _ := strconv.Atoi(fields[0])
		count, _ := strconv.Atoi(fields[1])
		st, _ := strconv.Atoi(fields[2])
		status := RangeStatus(st)

		if status == rangeStatusScanned {
			scannedRanges[start] = status
		} else {
			if status == rangeStatusBad {
				badRangesMap[start] = BlockRange{uint32(start), uint32(count)}
			} else {
				delete(badRangesMap, start)
			}
		}
	}

	file.Close()

	var notScannedRanges []BlockRange

	var i uint32
	for i < blocksCount {
		count := blocksCount - i
		if count > batch {
			count = batch
		}

		blockRange := BlockRange{i, count}
		i = i + count

		_, prs := scannedRanges[int(blockRange.StartIndex)]

		if !prs {
			notScannedRanges = append(notScannedRanges, blockRange)
		}
	}

	var badRanges []BlockRange
	for _, r := range badRangesMap {
		badRanges = append(badRanges, r)
	}

	return notScannedRanges, badRanges, nil
}

func alignRanges(blockRanges []BlockRange) []BlockRange {
	var alignedRanges []BlockRange

	for _, blockRange := range blockRanges {
		if blockRange.BlocksCount == 0 {
			continue
		}
		l := uint32(blockRange.StartIndex/1024) * 1024
		r := uint32((blockRange.StartIndex+blockRange.BlocksCount-1)/1024+1) * 1024
		alignedRanges = append(alignedRanges, BlockRange{l, r - l})
	}

	sort.SliceStable(alignedRanges, func(i, j int) bool {
		return alignedRanges[i].StartIndex < alignedRanges[j].StartIndex
	})

	var result []BlockRange
	result = append(result, alignedRanges[0])

	for _, alignedRange := range alignedRanges {
		r := result[len(result)-1]
		l1 := r.StartIndex
		r1 := l1 + r.BlocksCount
		l2 := alignedRange.StartIndex
		r2 := l2 + alignedRange.BlocksCount

		if r1 < l2 {
			result = append(result, alignedRange)
		} else if r1 < r2 {
			result[len(result)-1] = BlockRange{l1, r2 - l1}
		}
	}

	return result
}

func mountVolume(
	ctx context.Context,
	client *nbs.Client,
	diskId string,
) error {
	socketPath := getSocketPath(diskId)
	clientId := "blockstore-repair"

	_, err := client.StartEndpoint(
		ctx,
		socketPath,
		diskId,
		pprotos.EClientIpcType_IPC_GRPC,
		clientId,
		pprotos.EVolumeAccessMode_VOLUME_ACCESS_REPAIR,
		pprotos.EVolumeMountMode_VOLUME_MOUNT_LOCAL)

	return err
}

func unmountVolume(
	ctx context.Context,
	client *nbs.Client,
	diskId string,
) error {
	socketPath := getSocketPath(diskId)
	return client.StopEndpoint(ctx, socketPath)
}

func scanRangeAsync(
	ctx context.Context,
	client *nbs.Client,
	diskId string,
	blockRange BlockRange,
	out chan BlockRange,
	outBad chan BlockRange,
	outErr chan error) {

	request := &protos.TDescribeBlocksRequest{
		DiskId:      diskId,
		StartIndex:  blockRange.StartIndex,
		BlocksCount: blockRange.BlocksCount,
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
			var badRanges []BlockRange
			for _, r := range p.Ranges {
				badRanges = append(badRanges, BlockRange{r.BlockIndex, r.BlocksCount})
			}

			badRanges = alignRanges(badRanges)
			for _, r := range badRanges {
				outBad <- r
			}
		}
	}

	out <- blockRange
	outErr <- nil
}

func compactRange(
	ctx context.Context,
	client *nbs.Client,
	diskId string,
	blockRange BlockRange,
) error {

	request := &protos.TCompactRangeRequest{
		DiskId:      diskId,
		StartIndex:  blockRange.StartIndex,
		BlocksCount: blockRange.BlocksCount,
	}

	fmt.Printf("CopactRange [%v %v] started\n",
		blockRange.StartIndex,
		blockRange.BlocksCount)

	response, err := CompactRange(ctx, client, request)
	if err != nil {
		return err
	}

	operationId := response.OperationId

	checkTime := time.Tick(500 * time.Millisecond)
	timeout := time.After(10 * time.Second)
	for {
		select {
		case <-checkTime:
			request := &protos.TGetCompactionStatusRequest{
				DiskId:      diskId,
				OperationId: operationId,
			}

			response, err := GetCompactionStatus(ctx, client, request)
			if err != nil {
				return err
			}

			if response.IsCompleted {
				fmt.Printf("CopactRange [%v %v] completed\n",
					blockRange.StartIndex,
					blockRange.BlocksCount)
				return nil
			}

			fmt.Printf("CopactRange [%v %v] in progress: %v/%v\n",
				blockRange.StartIndex,
				blockRange.BlocksCount,
				response.Progress,
				response.Total)

		case <-timeout:
			return fmt.Errorf("Not compact range (timeout): [%v %v]",
				blockRange.StartIndex, blockRange.BlocksCount)
		}
	}
}

func repairMountedVolume(
	ctx context.Context,
	client *nbs.Client,
	diskId string,
	blocksCount uint32,
	batch uint32,
) error {

	dumpFile := fmt.Sprintf("./repair-%s.txt", diskId)

	fmt.Printf("LoadRanges from file %s\n", dumpFile)

	notScannedRanges, badRanges, err := restoreRanges(blocksCount, batch, dumpFile)

	if err != nil {
		return err
	}

	fmt.Printf("ScanVolume %s started. %v ranges to scan\n", diskId, len(notScannedRanges))

	out := make(chan BlockRange)
	outBad := make(chan BlockRange)
	outErr := make(chan error)

	pending := 0

	for _, r := range notScannedRanges {
		go scanRangeAsync(ctx, client, diskId, r, out, outBad, outErr)
		pending++
	}

	total := pending

	mountTime := time.Tick(5 * time.Second)

	for {
		select {
		case r := <-out:
			saveRange(r, rangeStatusScanned, dumpFile)
		case r := <-outBad:
			badRanges = append(badRanges, r)
			saveRange(r, rangeStatusBad, dumpFile)

		case err := <-outErr:
			if err != nil {
				return err
			}
			pending--
			if pending == 0 {
				break
			}

			fmt.Printf("ScanVolume %s in progress %v/%v, found %v bad ranges\n",
				diskId, total-pending, total, len(badRanges))

		case <-mountTime:
			err := mountVolume(ctx, client, diskId)
			if err != nil {
				return err
			}
		}
	}

	fmt.Printf("ScanVolume %s completed, found %v bad ranges\n",
		diskId, len(badRanges))

	fmt.Printf("RepairVolume %s started. %v ranges to repair\n",
		diskId, len(notScannedRanges))

	var repairedRanges []BlockRange

	for _, r := range badRanges {
		if err := mountVolume(ctx, client, diskId); err != nil {
			return err
		}

		if err := compactRange(ctx, client, diskId, r); err != nil {
			return err
		}

		repairedRanges = append(repairedRanges, r)

		fmt.Printf("RepairVolume %s in progress %v/%v\n",
			diskId, len(repairedRanges), len(badRanges))

		saveRange(r, rangeStatusRepaired, dumpFile)
	}

	fmt.Printf("RepairVolume %s completed, repaired %v ranges\n",
		diskId, len(repairedRanges))

	return nil
}

func RepairVolume(
	ctx context.Context,
	client *nbs.Client,
	diskId string,
	blocksCount uint32,
	batch uint32,
) error {

	if err := mountVolume(ctx, client, diskId); err != nil {
		return err
	}

	err := repairMountedVolume(
		ctx,
		client,
		diskId,
		blocksCount,
		batch)

	unmountVolume(ctx, client, diskId)

	return err
}
