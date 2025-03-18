package main

import (
	"flag"
)

type Args struct {
	Addr       string
	SleepDelay int64
}

func ParseArgs(inputArgs []string) (args Args) {
	flagSet := flag.NewFlagSet("captcha_cloud_api", flag.PanicOnError)

	flagSet.StringVar(&args.Addr, "addr", ":15000", "address to listen")
	flagSet.Int64Var(&args.SleepDelay, "sleep-delay", 0, "sleep delay")

	// Shouldn't return an error since we use flag.PanicOnError.
	_ = flagSet.Parse(inputArgs)

	return
}
