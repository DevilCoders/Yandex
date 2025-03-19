package main

import (
	"context"
	"errors"
	"log"
	"net/http"
	"time"

	"github.com/uber/jaeger-client-go/config"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/metrics"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracetag"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracing"
)

func main() {
	runTracing()
	go runMetrics()

	go logCycle()
	go loop()
	ctx := context.Background()
	<-ctx.Done()
}

func runMetrics() {
	http.Handle("/metrics", metrics.GetHandler())
	_ = http.ListenAndServe(":2112", nil)
}

func runTracing() {
	const trURL = "http://localhost:16686"

	cfg := config.Configuration{
		ServiceName: "simulateDev",

		Sampler: &config.SamplerConfig{
			Type: "const", Param: 1,
		},
		Reporter: &config.ReporterConfig{LocalAgentHostPort: ":6831"},
	}

	t, c, err := cfg.NewTracer()
	if err != nil {
		log.Fatalf("trace init error: %s", err.Error())
	}

	tracing.SetTracer(t, c)
}

func loop() {
	for {
		start()
		time.Sleep(time.Second)
	}
}

func start() {
	ctx := context.Background()
	ctx, cm := tooling.InitContext(ctx, "simulate")

	cm.InitTracing("simulation_handling").SetSource("test_source")
	defer cm.FinishTracing()
	handler(ctx)
}

func handler(ctx context.Context) {
	tooling.ModifyContext(ctx).SetHandler("test_handler")
	MainAction(ctx)
	ErrAction(ctx)
}

func MainAction(ctx context.Context) {
	ctx = tooling.ActionStarted(ctx)
	defer tooling.ActionDone(ctx, nil)

	ctx = tooling.StartRetry(ctx)
	for i := 1; i < 3; i++ {
		tooling.RetryIteration(ctx)
		GoodQuery(ctx)
		ErrQuery(ctx)
		GoodRequest(ctx)
		ErrRequest(ctx)
		time.Sleep(time.Millisecond * 100)
	}
}

func ErrAction(ctx context.Context) {
	ctx = tooling.ActionStarted(ctx)
	defer tooling.ActionDone(ctx, errors.New("error in action"))

	tooling.TraceTag(ctx, tracetag.Failed())
	tooling.TraceEvent(ctx, "custom test event", logf.Size(999))
}

func GoodQuery(ctx context.Context) {
	ctx = tooling.QueryStarted(ctx)
	defer tooling.QueryDone(ctx, nil)
}

func ErrQuery(ctx context.Context) {
	ctx = tooling.QueryStarted(ctx)
	defer tooling.QueryDone(ctx, errors.New("error in query"))
}

func GoodRequest(ctx context.Context) {
	ctx = tooling.ICRequestStarted(ctx, "rpc_system")
	defer tooling.ICRequestDone(ctx, nil)
}

func ErrRequest(ctx context.Context) {
	ctx = tooling.ICRequestStarted(ctx, "rpc_system")
	defer tooling.ICRequestDone(ctx, errors.New("error in rpc"))
}
