package logbroker

import (
	"context"
	"errors"
	"fmt"

	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/library/go/core/log"
)

func (c *consumer) handleEvent(ctx context.Context, e persqueue.Event) (forceCallback func() error, resultErr error) {
	if e == nil {
		return nil, nil
	}
	defer func() {
		if resultErr != nil {
			c.logger.Error("logbroker event handling error", log.Error(resultErr))
		}
	}()

	switch m := e.(type) {
	default: // if we don't know this message - it is problem for us
		c.logger.Error("unexpected logbroker event", log.Any("event", e))
		return nil, errFatal.Wrap(errors.New("got unexpected logbroker event"))
	case persqueue.DataMessage:
		return nil, c.handleData(ctx, m)
	case *persqueue.CommitAck: // we should not take actions on this message, but log it for debug
		c.logger.Debug("logbroker commits", log.UInt64s("cookies", m.Cookies))
		return nil, nil
	case *persqueue.Lock: // logbroker offers new partition to us, we can not decline
		return nil, c.handleLockV0(ctx, m)
	case *persqueue.LockV1: // logbroker offers new partition to us, we can not decline
		return nil, c.handleLockV1(ctx, m)
	case *persqueue.Release: // logbroker take partition away and no more data and commits
		return c.handleUnlockV0(ctx, m)
	case *persqueue.ReleaseV1: // logbroker take partition away and no more data and commits
		return c.handleUnlockV1(ctx, m)
	case *persqueue.Disconnect: // this event emitted when library reconnects after error
		return nil, c.handleDisconnect(ctx, m)
	}
}

func (c *consumer) handleData(_ context.Context, d persqueue.DataMessage) error {
	// group batches by source and stores as infly
	batches := d.Batches()
	sm := make([]sourceMessages, 0, len(batches))
	msgCount := 0
	sources := map[sourceKey]struct{}{}
	for _, b := range batches {
		src := source(b.Topic, b.Partition)
		msgCount += len(b.Messages)
		sources[src] = struct{}{}

		sm = append(sm, sourceMessages{
			src:      src,
			messages: b.Messages,
		})
	}

	c.logger.Debug("data", log.Int("sources", len(sources)), log.Int("messages", msgCount))
	if err := c.pushData(d, sm); err != nil {
		return errFatal.Wrap(err)
	}
	c.counters.inflyUpdated(c.size(), msgCount)

	return nil
}

type readStarter interface {
	StartRead(verifyReadOffset bool, readOffset, commitOffset uint64)
}

func (c *consumer) handleLockV0(ctx context.Context, l *persqueue.Lock) error {
	src := source(l.Topic, l.Partition)

	c.logger.Debug("lock message", log.String("lb_source", src.String()), log.UInt64("offset", l.ReadOffset), log.UInt64("generation", l.Generation))
	return c.handleLock(ctx, l, src, l.ReadOffset)
}

func (c *consumer) handleLockV1(ctx context.Context, l *persqueue.LockV1) error {
	src := source(l.Topic, uint32(l.Partition))

	c.logger.Debug("lock message v1",
		log.String("lb_source", src.String()), log.UInt64("offset", l.ReadOffset), log.String("cluster", l.Cluster), log.UInt64("assign_id", l.AssignID),
	)
	return c.handleLock(ctx, l, src, l.ReadOffset)
}

func (c *consumer) handleLock(ctx context.Context, rs readStarter,
	src sourceKey, lockReadOffset uint64,
) error {
	if err := c.registerSource(ctx, src); err != nil {
		c.logger.Error("lock registration issue", log.String("lb_source", src.String()), log.Error(err))
		return err
	}

	// we can store source offsets in some storage and get it back on partition locking
	offset, err := c.offsets.GetOffset(ctx, lbtypes.SourceID(src.String()))
	if err != nil {
		return fmt.Errorf("can not get lock offset for source %s: %w", src.String(), err)
	}

	// assume that we got correct maximum offset from previous read
	readOffset := offset + 1
	switch {
	case offset == 0: // if no offset - get it from lock
		c.logger.Debug("no offset for source", log.String("lb_source", src.String()))
		readOffset = lockReadOffset
	case readOffset < lockReadOffset: // if offset in logbroker moved forward then we loose data from readOffset to lockReadOffset
		// c.logger.Error("data loss detected",
		// 	log.String("lb_source", src.String()), log.UInt64("lb_offset", lockReadOffset), log.UInt64("stored_offset", readOffset),
		// )
		readOffset = lockReadOffset
	case readOffset > lockReadOffset:
		c.logger.Warn("offset rewind",
			log.String("lb_source", src.String()), log.UInt64("lb_offset", lockReadOffset), log.UInt64("stored_offset", readOffset),
		)
	}

	// all messages with offset less than passed commitOffset will be commited
	// if we don't want to commit something then set it to 0
	commitOffset := readOffset
	rs.StartRead(true, readOffset, commitOffset)
	c.counters.lockUpdated(len(c.sources), 1)

	c.logger.Info("ready to read",
		log.String("lb_source", src.String()), log.UInt64("lb_offset", lockReadOffset), log.UInt64("stored_offset", readOffset),
	)

	return nil
}

func (c *consumer) handleUnlockV0(ctx context.Context, r *persqueue.Release) (func() error, error) {
	src := source(r.Topic, r.Partition)

	c.logger.Info("release message", log.String("lb_source", src.String()), log.UInt64("generation", r.Generation))

	if !c.hasInfly(src) {
		return nil, c.handleUnlock(ctx, src)
	}
	if !r.CanCommit {
		return nil, fmt.Errorf("source %s can not be released due to inflies and deny of commits", src.String())
	}

	callback := func() error {
		return c.handleUnlock(ctx, src)
	}

	return callback, nil
}

func (c *consumer) handleUnlockV1(ctx context.Context, r *persqueue.ReleaseV1) (func() error, error) {
	src := source(r.Topic, uint32(r.Partition))

	// NOTE: assign id is in Generation field
	c.logger.Info("release message v1", log.String("lb_source", src.String()), log.String("cluster", r.Cluster), log.UInt64("assign_id", r.Generation))

	if !c.hasInfly(src) {
		defer r.Release()
		return nil, c.handleUnlock(ctx, src)
	}
	if !r.CanCommit {
		r.Release()
		return nil, fmt.Errorf("source %s can not be released due to inflies and deny of commits", src.String())
	}

	callback := func() error {
		defer r.Release()
		return c.handleUnlock(ctx, src)
	}

	return callback, nil
}

func (c *consumer) handleUnlock(_ context.Context, src sourceKey) error {
	if err := c.releaseSource(src); err != nil {
		c.logger.Warn("lock release issue", log.Error(err))
	}
	c.counters.lockUpdated(len(c.sources), 0)

	return nil
}

func (c *consumer) handleDisconnect(_ context.Context, d *persqueue.Disconnect) error {
	c.logger.Info("disconnect occurred", log.Error(d.Err))
	c.resetInfly()
	c.counters.readerReseted()
	return nil
}
