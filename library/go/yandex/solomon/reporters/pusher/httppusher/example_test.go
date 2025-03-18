package httppusher_test

import (
	"context"
	"os"
	"os/signal"
	"syscall"
	"time"

	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/pusher/httppusher"
)

func ExampleNewPusher() {
	// create metrics registry
	opts := solomon.NewRegistryOpts().
		SetSeparator('_').
		SetPrefix("myprefix")

	reg := solomon.NewRegistry(opts)

	// register new metric
	cnt := reg.Counter("cyclesCount")

	// pass metric to your function and do job
	go func() {
		for {
			cnt.Inc()
			time.Sleep(1 * time.Second)
		}
	}()

	// create new pusher with options
	popts := []httppusher.PusherOpt{
		httppusher.SetService("myservice"),
		httppusher.SetCluster("prod"),
		httppusher.SetProject("api"),
	}

	pusher, err := httppusher.NewPusher(popts...)
	if err != nil {
		panic(err)
	}

	// push manually
	metrics, err := reg.Gather()
	if err != nil {
		panic(err)
	}
	_ = pusher.Push(context.Background(), metrics)

	// or push automatically on interval
	ctx, cancel := context.WithCancel(context.Background())
	go func() {
		_ = pusher.Start(ctx, reg, 5*time.Second)
	}()

	// wait till Ctrl+C pressed to stop pusher
	stop := make(chan os.Signal, 1)
	signal.Notify(stop, syscall.SIGINT)
	<-stop
	cancel()
}
