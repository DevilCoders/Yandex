package common

import (
	"context"
)

////////////////////////////////////////////////////////////////////////////////

type ChannelWithFeedback struct {
	inflightQueue *InflightQueue
	channel       chan uint32
}

// Not thread-safe.
func (c *ChannelWithFeedback) Send(ctx context.Context, value uint32) error {
	select {
	case c.channel <- value:
	case <-ctx.Done():
		return ctx.Err()
	}

	return c.inflightQueue.Add(ctx, value)
}

func (c *ChannelWithFeedback) GetChannel() chan uint32 {
	return c.channel
}

func (c *ChannelWithFeedback) GetMilestone() uint32 {
	return c.inflightQueue.GetMilestone()
}

func (c *ChannelWithFeedback) Close() {
	close(c.channel)
}

////////////////////////////////////////////////////////////////////////////////

func CreateChannelWithFeedback(
	milestone uint32,
	feedback chan uint32,
) *ChannelWithFeedback {

	// TODO: should be passed explicitly via param.
	inflightLimit := cap(feedback)

	return &ChannelWithFeedback{
		inflightQueue: CreateInflightQueue(milestone, feedback),
		channel:       make(chan uint32, inflightLimit),
	}
}
