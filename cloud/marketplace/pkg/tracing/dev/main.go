package main

import (
	"context"
	"fmt"
	stdlog "log"
	"time"

	"github.com/spf13/cobra"

	"github.com/opentracing/opentracing-go/log"
	"github.com/uber/jaeger-client-go"
	jgrcfg "github.com/uber/jaeger-client-go/config"

	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
)

var (
	traceAgentEndpoint string
	withError          bool
)

var rootCmd = &cobra.Command{
	Use:   "dev",
	Short: "Run tracing setup check",

	Run: runTracingTest,
}

func main() {
	if err := rootCmd.Execute(); err != nil {
		stdlog.Fatal(err)
	}
}

func init() {
	rootCmd.Flags().StringVarP(&traceAgentEndpoint, "endpoint", "p", "", "jaeger agent endpoint")
	rootCmd.Flags().BoolVarP(&withError, "error", "e", false, "fail with error")
}

func foo(ctx context.Context) {
	span, traceCtx := tracing.Start(ctx, "foo_test1")
	defer span.Finish()

	bar(traceCtx)
}

func bar(ctx context.Context) {
	span, traceCtx := tracing.Start(ctx, "bar_test1")
	defer span.Finish()

	mayBeError(traceCtx)
}

func mayBeError(ctx context.Context) {
	span, _ := tracing.Start(ctx, "err_test1")
	defer span.Finish()

	if withError {
		span.LogFields(
			log.Error(fmt.Errorf("BOOM")),
		)

		span.SetTag("error", fmt.Errorf("BAM"))
	}
}

func runTracingTest(_ *cobra.Command, _ []string) {
	reporterConfig := jgrcfg.ReporterConfig{
		BufferFlushInterval: time.Second,
		LogSpans:            true,
		QueueSize:           10,
		LocalAgentHostPort:  traceAgentEndpoint,
	}

	cfg := jgrcfg.Configuration{
		ServiceName: "mkt-lich-test",
		Reporter:    &reporterConfig,
		Sampler: &jgrcfg.SamplerConfig{
			Type:  jaeger.SamplerTypeConst,
			Param: 1,
		},
	}

	opts := []jgrcfg.Option{
		jgrcfg.Logger(jaeger.StdLogger),
		jgrcfg.Tag("service.version", "41"),
	}

	tracer, closer, err := cfg.NewTracer(opts...)
	if err != nil {
		stdlog.Fatal(err)
	}

	tracing.SetTracer(tracer)

	fmt.Println("starting...")

	span := tracer.StartSpan("main.test")
	defer span.Finish()

	runCtx := context.Background()
	foo(runCtx)

	if err := closer.Close(); err != nil {
		stdlog.Fatal(err)
	}
}
