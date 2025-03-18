// Copyright 2016 Michal Witkowski. All Rights Reserved.
// See LICENSE for licensing terms.

/*
Package `grpctest` provides helper functions for testing validators in this package.
*/

package grpctest

import (
	"context"
	"io"
	"testing"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/test/grpctest/testproto"
)

const (
	// DefaultPongValue is the default value used.
	DefaultResponseValue = "default_response_value"
	// ListResponseCount is the expeted number of responses to PingList
	ListResponseCount = 100
)

type TestPingService struct {
	T *testing.T
}

func (s *TestPingService) PingEmpty(ctx context.Context, _ *testproto.Empty) (*testproto.PingResponse, error) {
	return &testproto.PingResponse{Value: DefaultResponseValue, Counter: 42}, nil
}

func (s *TestPingService) Ping(ctx context.Context, ping *testproto.PingRequest) (*testproto.PingResponse, error) {
	// Send user trailers and headers.
	return &testproto.PingResponse{Value: ping.Value, Counter: 42}, nil
}

func (s *TestPingService) PingError(ctx context.Context, ping *testproto.PingRequest) (*testproto.Empty, error) {
	code := codes.Code(ping.ErrorCodeReturned)
	return nil, status.Errorf(code, "Userspace error.")
}

func (s *TestPingService) PingList(ping *testproto.PingRequest, stream testproto.TestService_PingListServer) error {
	if ping.ErrorCodeReturned != 0 {
		return status.Errorf(codes.Code(ping.ErrorCodeReturned), "foobar")
	}
	// Send user trailers and headers.
	for i := 0; i < ListResponseCount; i++ {
		err := stream.Send(&testproto.PingResponse{Value: ping.Value, Counter: int32(i)})
		if err != nil {
			return err
		}
	}
	return nil
}

func (s *TestPingService) PingStream(stream testproto.TestService_PingStreamServer) error {
	count := 0
	for {
		ping, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			return err
		}
		err = stream.Send(&testproto.PingResponse{Value: ping.Value, Counter: int32(count)})
		if err != nil {
			return err
		}
		count += 1
	}
	return nil
}
