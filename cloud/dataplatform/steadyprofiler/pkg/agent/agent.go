package agent

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"math/rand"
	"net/http"
	"runtime/pprof"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ProfileType uint8

const (
	TypeUnknown ProfileType = iota
	TypeCPU
	TypeHeap
	TypeBlock
	TypeMutex
	TypeGoroutine
	TypeThreadcreate

	TypeOther ProfileType = 127
	TypeTrace ProfileType = 128
)

const (
	defaultProfileType = TypeCPU

	defaultDuration     = 10 * time.Second
	defaultTickInterval = time.Hour
)

func init() {
	rand.Seed(time.Now().UnixNano())
}

func (ptype ProfileType) String() string {
	switch ptype {
	case TypeUnknown:
		return "unknown"
	case TypeCPU:
		return "cpu"
	case TypeHeap:
		return "heap"
	case TypeBlock:
		return "block"
	case TypeMutex:
		return "mutex"
	case TypeGoroutine:
		return "goroutine"
	case TypeThreadcreate:
		return "threadcreate"
	case TypeOther:
		return "other"
	case TypeTrace:
		return "trace"
	}
	return fmt.Sprintf("ProfileType(%d)", ptype)
}

type profileRecord struct {
	ProfileType string
	Service     string
	Data        []byte
	Labels      map[string]string
	TS          time.Time
	TSNano      int64
}

func Start(service string, opts ...Option) (*Agent, error) {
	agent := New(service, opts...)
	if err := agent.Start(context.Background()); err != nil {
		return nil, err
	}
	return agent, nil
}

type httpClient interface {
	Do(req *http.Request) (*http.Response, error)
}

type Agent struct {
	CPUProfile          bool
	CPUProfileDuration  time.Duration
	HeapProfile         bool
	BlockProfile        bool
	MutexProfile        bool
	GoroutineProfile    bool
	ThreadcreateProfile bool
	service             string
	logger              log.Logger
	tick                time.Duration
	stop                chan struct{} // signals the beginning of stop
	done                chan struct{} // closed when stopping is done
	writerOptions       *persqueue.WriterOptions
	writer              persqueue.Writer
	labels              map[string]string
}

func New(service string, opts ...Option) *Agent {
	a := new(Agent)
	a.CPUProfile = true // enable CPU profiling by default
	a.CPUProfileDuration = defaultDuration
	a.service = service
	a.tick = defaultTickInterval
	a.stop = make(chan struct{})
	a.done = make(chan struct{})
	a.logger = logger.NullLog

	for _, opt := range opts {
		opt(a)
	}

	return a
}

func (a *Agent) Start(ctx context.Context) error {
	if a.writerOptions == nil {
		return xerrors.New("failed to start agent: persqueue writer options required")
	}
	writer := persqueue.NewWriter(*a.writerOptions)
	_, err := writer.Init(ctx)
	if err != nil {
		return err
	}
	a.writer = writer
	go a.collectAndSend(ctx)
	go persqueue.ReceiveIssues(a.writer)
	return nil
}

func (a *Agent) Stop() {
	close(a.stop)
	<-a.done
	_ = a.writer.Close()
}

func (a *Agent) collectProfile(ctx context.Context, ptype ProfileType, buf *bytes.Buffer) error {
	switch ptype {
	case TypeCPU:
		err := pprof.StartCPUProfile(buf)
		if err != nil {
			return xerrors.Errorf("failed to start CPU profile: %w", err)
		}
		sleep(a.CPUProfileDuration, ctx.Done())
		pprof.StopCPUProfile()
	case TypeHeap:
		err := pprof.WriteHeapProfile(buf)
		if err != nil {
			return xerrors.Errorf("failed to write heap profile: %w", err)
		}
	case TypeBlock,
		TypeMutex,
		TypeGoroutine,
		TypeThreadcreate:

		p := pprof.Lookup(ptype.String())
		if p == nil {
			return xerrors.Errorf("unknown profile type %v", ptype)
		}
		err := p.WriteTo(buf, 0)
		if err != nil {
			return xerrors.Errorf("failed to write %s profile: %w", ptype, err)
		}
	default:
		return xerrors.Errorf("unknown profile type %v", ptype)
	}

	return nil
}

func (a *Agent) sendProfile(ctx context.Context, ptype ProfileType, buf *bytes.Buffer) error {
	data, err := json.Marshal(profileRecord{
		ProfileType: ptype.String(),
		Service:     a.service,
		Data:        buf.Bytes(),
		Labels:      a.labels,
		TS:          time.Now(),
		TSNano:      time.Now().UnixNano(),
	})
	if err != nil {
		return err
	}
	return a.writer.Write(&persqueue.WriteMessage{Data: data})
}

func (a *Agent) collectAndSend(ctx context.Context) {
	defer close(a.done)

	ctx, cancel := context.WithCancel(ctx)
	go func() {
		<-a.stop
		cancel()
	}()

	var (
		ptype = a.nextProfileType(TypeUnknown)
		timer = time.NewTimer(tickInterval(0))

		buf bytes.Buffer
	)

	for {
		select {
		case <-a.stop:
			if !timer.Stop() {
				<-timer.C
			}
			return
		case <-timer.C:
			if err := a.collectProfile(ctx, ptype, &buf); err != nil {
				a.logger.Warn("unable to collect profiles", log.Error(err))
			} else if err := a.sendProfile(ctx, ptype, &buf); err != nil {
				a.logger.Warn("unable to send profiles", log.Error(err))
			}

			buf.Reset()

			ptype = a.nextProfileType(ptype)

			var tick time.Duration
			if ptype == defaultProfileType {
				// we took the full set of profiles, sleep for the whole tick
				tick = a.tick
			}

			timer.Reset(tickInterval(tick))
		}
	}
}

func (a *Agent) nextProfileType(ptype ProfileType) ProfileType {
	// special case to choose initial profile type on the first call
	if ptype == TypeUnknown {
		return defaultProfileType
	}

	for {
		switch ptype {
		case TypeCPU:
			ptype = TypeHeap
			if a.HeapProfile {
				return ptype
			}
		case TypeHeap:
			ptype = TypeBlock
			if a.BlockProfile {
				return ptype
			}
		case TypeBlock:
			ptype = TypeMutex
			if a.MutexProfile {
				return ptype
			}
		case TypeMutex:
			ptype = TypeGoroutine
			if a.GoroutineProfile {
				return ptype
			}
		case TypeGoroutine:
			ptype = TypeThreadcreate
			if a.ThreadcreateProfile {
				return ptype
			}
		case TypeThreadcreate:
			ptype = TypeCPU
			if a.CPUProfile {
				return ptype
			}
		}
	}
}

func tickInterval(d time.Duration) time.Duration {
	// add up to extra 10 seconds to sleep to dis-align profiles of different instances
	noise := time.Second + time.Duration(rand.Intn(9))*time.Second
	return d + noise
}

var timersPool = sync.Pool{}

func sleep(d time.Duration, cancel <-chan struct{}) {
	timer, _ := timersPool.Get().(*time.Timer)
	if timer == nil {
		timer = time.NewTimer(d)
	} else {
		timer.Reset(d)
	}

	select {
	case <-timer.C:
	case <-cancel:
		if !timer.Stop() {
			<-timer.C
		}
	}

	timersPool.Put(timer)
}
