package common

import (
	"context"
	"sync/atomic"
)

////////////////////////////////////////////////////////////////////////////////

type InflightQueue struct {
	milestone uint32
	processed chan uint32
	inflight  []uint32
}

func (c *InflightQueue) valueProcessed(value uint32) {
	c.inflight = Remove(c.inflight, value)

	if len(c.inflight) != 0 {
		atomic.StoreUint32(&c.milestone, c.inflight[0])
	}
}

func (c *InflightQueue) drainInflight(ctx context.Context) error {
	// Drain synchronously until limit is met.
	for len(c.inflight) >= cap(c.inflight) {
		select {
		case value := <-c.processed:
			c.valueProcessed(value)
		case <-ctx.Done():
			return ctx.Err()
		}
	}

	// Drain asynchronously.
	for {
		select {
		case value := <-c.processed:
			c.valueProcessed(value)
		case <-ctx.Done():
			return ctx.Err()
		default:
			return nil
		}
	}
}

// Not thread-safe.
func (c *InflightQueue) Add(ctx context.Context, value uint32) error {
	c.inflight = append(c.inflight, value)

	return c.drainInflight(ctx)
}

func (c *InflightQueue) GetMilestone() uint32 {
	return atomic.LoadUint32(&c.milestone)
}

////////////////////////////////////////////////////////////////////////////////

func CreateInflightQueue(
	milestone uint32,
	processed chan uint32,
) *InflightQueue {

	// TODO: should be passed explicitly via param.
	inflightLimit := cap(processed)

	return &InflightQueue{
		milestone: milestone,
		processed: processed,
		inflight:  make([]uint32, 0, inflightLimit),
	}
}
