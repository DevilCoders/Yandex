package yc

import (
	"google.golang.org/grpc"
)

type Client struct {
	cc grpc.ClientConnInterface
}

func NewClient(token string) (*Client, error) {
	g, err := newGRPCClient(token)
	if err != nil {
		return nil, err
	}

	c, err := g.getConn()
	if err != nil {
		return nil, err
	}

	return &Client{cc: c}, nil
}
