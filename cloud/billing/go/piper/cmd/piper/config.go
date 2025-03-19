package main

import (
	"context"
	"log"
	"path/filepath"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config"
)

var mainCtx, globalStop = context.WithCancel(context.Background())

func getContainer(override config.OverrideFlag) *config.Container {
	var configFiles []string

	for _, p := range configPaths {
		paths, err := filepath.Glob(p)
		if err != nil {
			log.Fatalf("error in path '%s'", p)
		}
		configFiles = append(configFiles, paths...)
	}

	return config.NewContainer(mainCtx, override, configFiles...)
}

func shutdownContainer(c *config.Container) {
	if err := c.Shutdown(); err != nil {
		log.Printf("shutdown error: %s", err.Error())
	}
}
