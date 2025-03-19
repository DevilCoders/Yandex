package logbroker

import (
	"context"
	"errors"
	"fmt"
	"sync"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
)

// committer can commit logbroker message batch
type committer interface {
	Commit()
}

// inflyMessages is storage for logbroker data during accumulation and handling messages
type inflyMessages struct {
	mu sync.Mutex

	// registered data sources with contexts for handling
	// when we detects source unlock we should cancel it's context
	sources map[sourceKey]sourceContext

	// stores instances that can commit ingflight messages
	commiters []committer
	// stores data messages grouped by source
	messages map[sourceKey][]persqueue.ReadMessage
}

// resetInfly cleans and recreates internal structures
func (i *inflyMessages) resetInfly() {
	defer lock(&i.mu).Unlock()
	if i.sources == nil {
		i.sources = make(map[sourceKey]sourceContext)
	}
	if i.messages == nil {
		i.messages = make(map[sourceKey][]persqueue.ReadMessage)
	}

	for s, c := range i.sources {
		c.cancel()
		delete(i.sources, s)
	}

	i.commiters = nil
	for src := range i.messages {
		delete(i.messages, src)
	}
}

// registerSource in inflight storage
//  - ctx - parent context for source context creation
//  - src - source key
//
func (i *inflyMessages) registerSource(ctx context.Context, src sourceKey) (err error) {
	defer lock(&i.mu).Unlock()

	if srcCtx, ok := i.sources[src]; ok {
		srcCtx.cancel()
		err = errors.New("source already locked") // here error does not cause registration cancellation
	}

	i.sources[src] = i.newSrcContext(ctx)
	i.messages[src] = i.messages[src][:0]
	return
}

// releaseSource from storage, drops it's messages if any and cancels it's context
func (i *inflyMessages) releaseSource(src sourceKey) error {
	defer lock(&i.mu).Unlock()
	srcCtx, ok := i.sources[src]
	if !ok {
		return errors.New("source is not locked")
	}
	if len(i.messages[src]) > 0 {
		return errors.New("source has infly messages")
	}

	srcCtx.cancel()
	delete(i.sources, src)
	delete(i.messages, src)
	return nil
}

// hasInfly check if source has infly messages
func (i *inflyMessages) hasInfly(src sourceKey) bool {
	defer lock(&i.mu).Unlock()
	return len(i.messages[src]) > 0
}

// pushData to storage
func (i *inflyMessages) pushData(cm committer, msg []sourceMessages) error {
	defer lock(&i.mu).Unlock()

	for _, m := range msg {
		if _, ok := i.sources[m.src]; !ok {
			return fmt.Errorf("data source %s is not locked", m.src.String())
		}
		i.messages[m.src] = append(i.messages[m.src], m.messages...)
	}
	i.commiters = append(i.commiters, cm)

	return nil
}

// checkLimit checks if storage contains more than limit messages by count or message size
func (i *inflyMessages) checkLimit(l int, s int) bool {
	checkSize := s > 0
	for _, m := range i.messages {
		l -= len(m)
		if l <= 0 {
			return true
		}
		if !checkSize {
			continue
		}
		for _, d := range m {
			s -= len(d.Data)
			if s <= 0 {
				return true
			}
		}
	}
	return false
}

// size return total infly messages count
func (i *inflyMessages) size() (sz int) {
	for _, m := range i.messages {
		sz += len(m)
	}
	return
}

// getHandlingParams - split accumulated data by sources, adds context and send that to handling
func (i *inflyMessages) getHandlingParams() (params []sourceHandleParams, commiters []committer) {
	defer lock(&i.mu).Unlock()

	params = make([]sourceHandleParams, 0, len(i.messages))
	for s, m := range i.messages {
		if len(m) == 0 {
			continue
		}
		params = append(params, sourceHandleParams{
			ctx: i.sources[s].ctx,
			sourceMessages: sourceMessages{
				src:      s,
				messages: m,
			},
		})
		i.messages[s] = nil
	}
	commiters = append(commiters, i.commiters...)
	i.commiters = nil

	return params, commiters
}

func (*inflyMessages) newSrcContext(ctx context.Context) sourceContext {
	result := sourceContext{}

	result.ctx, result.cancel = context.WithCancel(ctx)
	return result
}

// helper types

type sourceContext struct {
	ctx    context.Context
	cancel context.CancelFunc
}

type sourceMessages struct {
	src      sourceKey
	messages []persqueue.ReadMessage
}

type sourceHandleParams struct {
	ctx context.Context
	sourceMessages
}
