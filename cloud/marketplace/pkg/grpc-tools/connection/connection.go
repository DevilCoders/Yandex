package connection

import (
	"context"

	"google.golang.org/grpc"
)

func Connection(ctx context.Context, endpoint string, options ...Option) (*grpc.ClientConn, error) {
	config := defaultConfig

	config.endpoint = endpoint

	for _, option := range options {
		option.apply(&config)
	}

	if err := config.validate(); err != nil {
		return nil, err
	}

	connOptions, err := config.generateDialOptions()
	if err != nil {
		return nil, err
	}

	ctx, cancel := context.WithTimeout(ctx, config.initTimeout)
	defer cancel()

	return grpc.DialContext(
		ctx,
		config.endpoint,
		connOptions...,
	)
}
