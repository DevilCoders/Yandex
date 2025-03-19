package main

import (
	"context"
	"fmt"
	"os"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/sender/pkg/app"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
)

var (
	rawBillType string
	showHelp    bool
)

func init() {
	pflag.StringVar(&rawBillType, "billing-type", rawBillType, "Send rawBillType metrics")
	pflag.BoolVarP(&showHelp, "help", "h", false, "Show help message")
	flags.RegisterConfigPathFlagGlobal()
	pflag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage of %s:\n", os.Args[0])
		pflag.PrintDefaults()
	}
}

func main() {
	pflag.Parse()
	if showHelp {
		pflag.Usage()
		return
	}

	billingType, err := billingdb.ParseBillType(rawBillType)
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to parse billing type: %+v\n", err)
		os.Exit(3)
	}

	os.Exit(app.RunLBBillingMetricsSender(context.Background(), billingType))
}
