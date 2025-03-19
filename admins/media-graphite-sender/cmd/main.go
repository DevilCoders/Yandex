package main

import (
	"flag"
	"log"
	"runtime"

	"a.yandex-team.ru/admins/media-graphite-sender/sender"
)

var args = &sender.Arguments{}

func init() {
	flag.BoolVar(&args.LogTrace, "log.trace", false, "Log incomming metrics")
	flag.StringVar(&args.Config, "config", "/etc/graphite-sender/config.yaml", "Config path")
	flag.Parse()
}

func main() {
	runtime.GOMAXPROCS(2)
	if err := sender.Run(args); err != nil {
		log.Fatalf("sender.Run failed: %v", err)
	}
}
