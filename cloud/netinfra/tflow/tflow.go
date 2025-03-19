package main

import (
	"context"
	"net/http"
	"os"
	"os/signal"
	"runtime"
	"sync"
	"syscall"

	"a.yandex-team.ru/cloud/netinfra/tflow/collector"
	"a.yandex-team.ru/cloud/netinfra/tflow/config"
	"a.yandex-team.ru/cloud/netinfra/tflow/consumer"
	"a.yandex-team.ru/cloud/netinfra/tflow/metrics"
	"a.yandex-team.ru/library/go/core/log"

	"github.com/prometheus/client_golang/prometheus/promhttp"
)

var (
	cfg    *config.Config
	logger log.Logger
)

type service interface {
	Run()
	Shutdown()
}

func main() {
	cfg, logger = config.GetOptions()

	var (
		wg                  sync.WaitGroup
		gracefulStopChannel = make(chan os.Signal, 1)
		reopenFiles         = make(chan os.Signal, 1)
		services            = []service{}
	)

	signal.Notify(gracefulStopChannel, syscall.SIGINT, syscall.SIGTERM)
	signal.Notify(reopenFiles, syscall.SIGUSR1)

	numberOfCPU := runtime.NumCPU()
	runtime.GOMAXPROCS(numberOfCPU)
	logger.Infof("%d cores available\n", numberOfCPU)

	//TODO: configure address from cfg not only port
	httpServer := &http.Server{Addr: cfg.StatsHTTPPort}
	http.Handle("/metrics", promhttp.Handler())
	go func() {
		err := httpServer.ListenAndServe()
		if err != nil {
			logger.Errorf("httpServer.ListenAndServe(): %s", err.Error())
		}
		defer func() {
			err := httpServer.Shutdown(context.TODO())
			if err != nil {
				logger.Errorf("httpServer.Shutdown(): %v", err)
			}
		}()
	}()

	cons := consumer.New(cfg.ConsumerConfig, logger)
	services = append(services, cons)

	// sFlow Collector
	if cfg.SFlowConfig.Cfg.Enabled {
		c, err := collector.NewSFlow(cfg.SFlowConfig, cons, logger)
		if err != nil {
			panic(err.Error())
		}
		services = append(services, service(c))
	}

	// ipfix collector
	if cfg.IpfixConfig.Cfg.Enabled {
		c, err := collector.NewIpfix(cfg.IpfixConfig, cons, logger)
		if err != nil {
			panic(err.Error())
		}
		services = append(services, service(c))
	}

	// Start services
	for _, s := range services {
		go s.Run()
	}

loop:
	for {
		select {
		case <-gracefulStopChannel:
			break loop
		case <-reopenFiles:
			cons.Reopen <- true
		}
	}

	// Graceful stop for services
	for i := len(services) - 1; i >= 0; i-- {
		s := services[i]
		wg.Add(1)
		go func(s service, wg *sync.WaitGroup) {
			defer wg.Done()
			s.Shutdown()
		}(s, &wg)
	}
	wg.Wait()
	metrics.Metrics.Dump()
	err := os.Remove(cfg.PIDFile)
	if err != nil {
		logger.Errorf("os.Remove(): remove PID file: %v", err)
	}
}
