package main

import (
	"context"
	"log"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config"
)

var mainCtx, globalStop = context.WithCancel(context.Background())

func getContainer() *config.Container {
	return config.NewContainer(mainCtx, config.WithOverride, configFiles...)
}

func shutdownContainer(c *config.Container) {
	if err := c.Shutdown(); err != nil {
		log.Printf("shutdown error: %s", err.Error())
	}
}
