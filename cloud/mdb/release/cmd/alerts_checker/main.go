package main

import (
	"context"
	"fmt"
	"time"

	"github.com/prometheus/client_golang/api"
	v1 "github.com/prometheus/client_golang/api/prometheus/v1"
	"github.com/prometheus/common/model"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var (
	prometheusAddress string
	targetNamespace   string
)

func init() {
	flags := pflag.NewFlagSet("Alerts Checker", pflag.ExitOnError)
	flags.StringVar(&prometheusAddress, "prometheus_address", "", "Prometheus address")
	flags.StringVar(&targetNamespace, "target_namespace", "", "Retrieve alerts for this namespace only")
	pflag.CommandLine.AddFlagSet(flags)
}

func main() {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	pflag.Parse()

	if len(prometheusAddress) == 0 {
		l.Error("Prometheus address required")
		return
	}

	if len(targetNamespace) == 0 {
		l.Error("Target namespace required")
		return
	}

	c, err := api.NewClient(api.Config{Address: prometheusAddress})
	if err != nil {
		panic(err)
	}

	v1api := v1.NewAPI(c)
	backoff := retry.New(retry.Config{MaxRetries: 10})

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Minute)
	defer cancel()

	var alerts v1.AlertsResult
	if err = backoff.RetryWithLog(ctx, func() error {
		ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
		defer cancel()

		alerts, err = v1api.Alerts(ctx)
		if err != nil {
			return err
		}

		return nil
	}, "couldn't retrieve errors", l); err != nil {
		return
	}

	var res v1.AlertsResult
	for _, alert := range alerts.Alerts {
		if hasLabel(alert.Labels, "kubernetes_namespace", targetNamespace) || hasLabel(alert.Labels, "namespace", targetNamespace) {
			res.Alerts = append(res.Alerts, alert)
		}
	}

	fmt.Printf("%+v\n", res)
}

func hasLabel(labels model.LabelSet, name, target string) bool {
	value, ok := labels[model.LabelName(name)]
	if !ok {
		return false
	}

	return string(value) == target
}
