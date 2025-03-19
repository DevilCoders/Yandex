package kmslbclient

import (
	"context"
	"fmt"
	"math"
	"math/rand"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

type expRetryPolicy struct {
	options RetryOptions
}

func (e expRetryPolicy) DoRetry(err error, numRetry int) (bool, time.Duration) {
	if numRetry >= e.options.MaxRetries-1 {
		return false, time.Duration(0)
	}
	if st := status.Code(err); st != codes.Unavailable && st != codes.ResourceExhausted && st != codes.DeadlineExceeded {
		return false, time.Duration(0)
	}
	jitterMult := 1.0
	if e.options.Jitter > 0.0 {
		jitterMult -= e.options.Jitter * rand.Float64()
	}
	mult := float64(e.options.InitialBackoff) * math.Pow(e.options.BackoffMultiplier, float64(numRetry)) * jitterMult
	duration := math.Max(float64(e.options.MaxBackoff), mult)
	return true, time.Duration(duration)
}

type RetryPolicy interface {
	DoRetry(err error, numRetry int) (bool, time.Duration)
}

func NewExponentialRetryPolicy(options RetryOptions) RetryPolicy {
	return &expRetryPolicy{
		options: options,
	}
}

func CallWithRetry(ctx context.Context, b Balancer, p RetryPolicy, handler func(conn *grpc.ClientConn) (interface{}, error)) (interface{}, error) {
	callState, err := b.NewCall()
	if err != nil {
		return nil, err
	}
	defer callState.Close()
	for numRetry := 0; ; {
		conn, err := callState.NextConn()
		if err != nil {
			return nil, err
		}
		ret, err := handler(conn)
		if err == nil {
			return ret, nil
		}
		doRetry, timeout := p.DoRetry(err, numRetry)
		if doRetry {
			select {
			case <-time.After(timeout):
				continue
			case <-ctx.Done():
				return nil, fmt.Errorf("context done: %s after %v retries. Last error: %w", ctx.Err(), numRetry, err)
			}
		}
		return ret, err
	}
}
