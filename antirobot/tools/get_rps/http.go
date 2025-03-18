package main

import (
	"context"
	"github.com/labstack/echo/v4"
	nethttp "net/http"
	"time"
)

type ServerConfig struct {
	Address     string `yaml:"address"`
	ReadTimeout string `yaml:"read_timeout,omitempty"`
	WriteTimeot string `yaml:"write_timeout,omitempty"`
}

type ServerContext struct {
	server   *nethttp.Server
	err      error
	stopChan chan struct{}
}

func RunServer(config ServerConfig, handler nethttp.Handler) (*ServerContext, error) {
	readTimeout, err := time.ParseDuration(config.ReadTimeout)
	if err != nil {
		return nil, err
	}

	writeTimeout, err := time.ParseDuration(config.WriteTimeot)
	if err != nil {
		return nil, err
	}

	ctx := &ServerContext{
		server: &nethttp.Server{
			Addr:         config.Address,
			ReadTimeout:  readTimeout,
			WriteTimeout: writeTimeout,
			Handler:      handler,
		},
		stopChan: make(chan struct{}),
	}

	go func() {
		defer close(ctx.stopChan)

		err := ctx.server.ListenAndServe()

		if err != nethttp.ErrServerClosed {
			ctx.err = err
		}
	}()

	return ctx, nil
}

func (c *ServerContext) Shutdown(ctx context.Context) error {
	return c.server.Shutdown(ctx)
}

func (c *ServerContext) Err() error {
	return c.err
}

func (c *ServerContext) Stopped() <-chan struct{} {
	return c.stopChan
}

func NewHealthCheckHTTPHandler() echo.HandlerFunc {
	return func(ctx echo.Context) error {
		return ctx.String(nethttp.StatusOK, "pong")
	}
}
