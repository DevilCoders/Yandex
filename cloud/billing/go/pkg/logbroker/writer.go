package logbroker

import (
	"context"
	"errors"
	"fmt"
	"strings"
	"sync"

	"golang.org/x/sync/errgroup"
	"golang.org/x/sync/semaphore"

	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
)

// NewShardProducer constructs new logbroker producer with sharding write logic.
func NewShardProducer(
	cfg persqueue.WriterOptions, route string, shardPartitions int, maxMessageSize int, maxParallel int,
) (lbtypes.ShardProducer, error) {
	if cfg.PartitionGroup != 0 {
		return nil, ErrMissconfigured.Wrap(errors.New("PartitionGroup should not be setted"))
	}
	if len(cfg.SourceID) != 0 {
		return nil, ErrMissconfigured.Wrap(errors.New("SourceID should not be setted"))
	}
	if route == "" {
		return nil, ErrMissconfigured.Wrap(errors.New("route is empty"))
	}
	if shardPartitions == 0 {
		return nil, ErrMissconfigured.Wrap(errors.New("shard partitions should be greater than 0"))
	}

	if maxParallel == 0 {
		maxParallel = shardPartitions
	}

	return &producerFabric{
		cfg:             cfg,
		route:           route,
		shardPartitions: shardPartitions,
		maxMessageSize:  maxMessageSize,
		sem:             semaphore.NewWeighted(int64(maxParallel)),
	}, nil
}

const (
	producerSourceVersion = "13"
	sourceType            = "logbroker-grpc" // if other sources will appear then need to change interfaces
)

type producerFabric struct {
	// NOTE: offset of original source message stores in SequenceNum increased by 1.
	// So this transformations everythere in this type.

	cfg persqueue.WriterOptions

	route           string
	shardPartitions int
	maxMessageSize  int

	sem *semaphore.Weighted

	queueOverride func(persqueue.WriterOptions) persqueue.Writer // For tests
}

func (p *producerFabric) GetOffset(ctx context.Context, srcID lbtypes.SourceID) (uint64, error) {
	if p.shardPartitions <= 0 {
		return 0, ErrMissconfigured.Wrap(errors.New("sharding partitions unknown"))
	}

	// Max offset for source id will be returned on writer init. So initializing writers
	// for every possible writer source id
	sourceName := string(srcID)
	srcIDS := make([]string, p.shardPartitions)

	if p.shardPartitions == 1 {
		srcIDS[0] = p.buildSourceID(p.route, sourceName)
	} else {
		for prt := range srcIDS {
			srcIDS[prt] = p.buildPartitionSourceID(p.route, prt, sourceName)
		}
	}

	seqNos := make([]uint64, len(srcIDS))

	eg, ctx := errgroup.WithContext(ctx)

	for idx, id := range srcIDS {
		idx := idx
		id := id
		eg.Go(func() error {
			if err := p.sem.Acquire(ctx, 1); err != nil {
				return err
			}
			defer p.sem.Release(1)

			w := p.makeWriterForPartition(id, uint32(idx))
			resp, err := w.Init(ctx)
			if err != nil {
				return ErrWriterInit.Wrap(err)
			}
			_ = w.Close()
			seqNos[idx] = resp.MaxSeqNo
			return nil
		})
	}

	if err := eg.Wait(); err != nil {
		return 0, err
	}

	seqNo := uint64(0)
	for _, sn := range seqNos {
		if sn > 0 && (seqNo == 0 || sn < seqNo) {
			seqNo = sn
		}
	}

	if seqNo > 0 { // see type note
		return seqNo - 1, nil
	}
	return 0, nil
}

func (p *producerFabric) Write(
	ctx context.Context, srcID lbtypes.SourceID, partition uint32, messages []lbtypes.ShardMessage,
) (maxOffset uint64, err error) {
	// NOTE: maxOffset==0 is special case - no data writes was performed.
	// It happens because no ack for some write was present and no update to value performed.

	if p.shardPartitions <= 0 {
		return 0, ErrMissconfigured.Wrap(errors.New("sharding partitions unknown"))
	}

	if partition > uint32(p.shardPartitions)-1 {
		return 0, ErrWrite.Wrap(fmt.Errorf("no such partition %d", partition))
	}

	sourceName := string(srcID)
	id := p.buildSourceID(p.route, sourceName)
	if p.shardPartitions > 1 {
		id = p.buildPartitionSourceID(p.route, int(partition), sourceName)
	}

	// we limit parallel writes through one producer
	if err := p.sem.Acquire(ctx, 1); err != nil {
		return 0, err
	}
	defer p.sem.Release(1)

	w := p.makeWriterForPartition(id, partition)
	resp, err := w.Init(ctx)
	if err != nil {
		return 0, ErrWriterInit.Wrap(err)
	}
	defer func() {
		// close is idempotent, here for strange cases
		_ = w.Close()
	}()

	// Sequence starts from 1 and if we got 0 - there are no previous writes.
	// We should consider this case.
	hasOffset := resp.MaxSeqNo > 0
	writtenOffset := resp.MaxSeqNo
	if hasOffset {
		writtenOffset--
	}

	wg := sync.WaitGroup{}
	wg.Add(1)

	var issues []*persqueue.Issue
	respCh := w.C()

	// collect acks and issues from writer.
	// if we does not do so - reader will be deadlocked.
	go func() {
		defer wg.Done()
		for {
			select {
			case wr, read := <-respCh:
				if !read {
					return
				}
				switch m := wr.(type) {
				case *persqueue.Ack:
					if maxOffset < m.SeqNo-1 {
						maxOffset = m.SeqNo - 1
					}
				case *persqueue.Issue:
					issues = append(issues, m)
				}
			case <-ctx.Done():
				return
			}
		}
	}()

	// spliting incoming messages by data size separate context used for stop data generation
	splitCtx, splitCancel := context.WithCancel(ctx)
	defer splitCancel()

	var marshalErr error
	callback := func(e error) {
		marshalErr = e
		splitCancel()
	}

	msgCh := p.prepareData(splitCtx, hasOffset, writtenOffset, messages, callback)
	for msg := range msgCh {
		if splitCtx.Err() == nil {
			wm := (&persqueue.WriteMessage{Data: msg.data}).WithSeqNo(msg.offset + 1)
			if writeErr := w.Write(wm); writeErr != nil {
				err = ErrWrite.Wrap(writeErr)
				splitCancel()
			}
		}
	}
	switch {
	case err != nil:
	case marshalErr != nil:
		err = ErrWrite.Wrap(marshalErr)
	case splitCtx.Err() != nil:
		err = splitCtx.Err()
	}
	if cErr := w.Close(); cErr != nil && err == nil {
		err = ErrWrite.Wrap(cErr)
	}

	wg.Wait()
	err = withIssues(err, issues)

	return
}

func (p *producerFabric) PartitionsCount() uint32 {
	return uint32(p.shardPartitions)
}

// buildSourceID constructs SourceID in following form:
//   <source_version>/<route>/<source_type>/<source_name>
func (producerFabric) buildSourceID(route, sourceName string) string {
	b := strings.Builder{}
	_, _ = b.WriteString(producerSourceVersion)
	if !strings.HasPrefix(route, "/") {
		_, _ = b.WriteRune('/')
	}
	_, _ = b.WriteString(route)
	_, _ = b.WriteRune('/')
	_, _ = b.WriteString(sourceType)
	if !strings.HasPrefix(sourceName, "/") {
		_, _ = b.WriteRune('/')
	}
	_, _ = b.WriteString(sourceName)
	return b.String()
}

// buildPartitionSourceID constructs SourceID in following form:
//   <source_version>/<route>.<partition>/<source_type>/<source_name>
func (p *producerFabric) buildPartitionSourceID(route string, partition int, sourceName string) string {
	return p.buildSourceID(fmt.Sprintf("%s.%d", route, partition), sourceName)
}

func (p *producerFabric) makeWriterForPartition(source string, partition uint32) persqueue.Writer {
	cfg := p.cfg
	if p.shardPartitions > 1 {
		cfg.PartitionGroup = partition + 1
	}
	cfg.SourceID = []byte(source)

	constructor := persqueue.NewWriter
	if p.queueOverride != nil {
		constructor = p.queueOverride
	}
	return constructor(cfg)
}

// prepareData join incoming messages into groups and merge them into one message with size check.
func (p *producerFabric) prepareData(
	ctx context.Context, hasOffset bool, writtenOffset uint64, messages []lbtypes.ShardMessage, callback func(err error),
) <-chan writeMessage {
	outChan := make(chan writeMessage, 1)
	if len(messages) == 0 { // No messages
		close(outChan)
		return outChan
	}

	if p.maxMessageSize <= 0 { // size unlimited - all in one message
		output := writeBuffers.Get()
		defer writeBuffers.Put(output)

		offset := uint64(0)
		for _, m := range messages {
			off := m.Offset()
			if hasOffset && off <= writtenOffset {
				continue
			}
			data, err := m.Data()
			if err != nil {
				callback(err)
				close(outChan)
				return outChan
			}
			writeWithLineEnd(output, data)
			offset = off
		}
		if output.Len() > 0 {
			outChan <- writeMessage{data: readAll(output), offset: offset}
		}
		close(outChan)
		return outChan
	}

	go func() {
		defer close(outChan)

		output := writeBuffers.Get()
		defer writeBuffers.Put(output)
		group := writeBuffers.Get()
		defer writeBuffers.Put(group)

		var curOffset, outputOffset uint64

		for i, m := range messages {
			if ctx.Err() != nil {
				return
			}
			off := m.Offset()
			if i == 0 {
				curOffset = off
			}

			if hasOffset && off <= writtenOffset {
				continue
			}

			if off != curOffset {
				_, _ = group.WriteTo(output)
				outputOffset = curOffset
				group.Reset()
			}

			data, err := m.Data()
			if err != nil {
				callback(err)
				return
			}
			writeWithLineEnd(group, data)
			curOffset = off

			if output.Len() > 0 && output.Len()+group.Len() > p.maxMessageSize {
				msg := writeMessage{data: readAll(output), offset: outputOffset}
				select {
				case outChan <- msg:
				case <-ctx.Done():
					return
				}
				output.Reset()
				outputOffset = 0
			}
		}
		if group.Len() != 0 || output.Len() != 0 {
			_, _ = group.WriteTo(output)
			msg := writeMessage{data: readAll(output), offset: curOffset}
			select {
			case outChan <- msg:
			case <-ctx.Done():
				return
			}
		}
	}()

	return outChan
}

type writeMessage struct {
	data   []byte
	offset uint64
}
