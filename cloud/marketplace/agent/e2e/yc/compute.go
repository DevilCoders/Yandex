package yc

import (
	"context"
	"fmt"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/compute/v1"
)

func (yc *Client) Compute() *Compute {
	return &Compute{cc: yc.cc}
}

type Compute struct {
	cc grpc.ClientConnInterface
}

func (c *Compute) GetInstanceByID(id string) (*compute.Instance, error) {
	s := compute.NewInstanceServiceClient(c.cc)
	r := compute.GetInstanceRequest{
		InstanceId: id,
		View:       compute.InstanceView_FULL,
	}

	ctx, cancel := context.WithTimeout(context.Background(), defaultTimeout)
	defer cancel()

	i, err := s.Get(ctx, &r)
	if err != nil {
		return nil, fmt.Errorf("%wget instance failed", err)
	}

	return i, nil
}
