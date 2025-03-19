package grpc

import (
	"context"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	now   = time.Now()
	later = time.Now().Add(time.Hour)
)

func TestGetsToken(t *testing.T) {
	credentials := NewTokenCredentials(func(ctx context.Context) (token iam.Token, err error) {
		return iam.Token{
			Value:     "testIamToken",
			ExpiresAt: later,
		}, nil
	})
	metadata, err := credentials.GetRequestMetadata(context.Background(), "")
	assert.NoError(t, err)
	assert.Equal(t, "Bearer "+"testIamToken", metadata["Authorization"])
}

func TestCachedToken_Token(t *testing.T) {
	t.Run("cache works", func(t *testing.T) {
		ts := &cachedToken{
			tokenF: func(context.Context) (iam.Token, error) {
				return iam.Token{
					Value:     "iamToken",
					ExpiresAt: later,
				}, nil
			},
			nowF: func() time.Time { return now },
		}
		gotToken, err := ts.Token(context.Background())
		assert.NoError(t, err)
		assert.Equal(t, "iamToken", gotToken)
		//second time
		gotToken, err = ts.Token(context.Background())
		assert.NoError(t, err)
		assert.Equal(t, "iamToken", gotToken)
	})
	t.Run("cache expires", func(t *testing.T) {
		i := 0
		ts := &cachedToken{
			tokenF: func(context.Context) (iam.Token, error) {
				if i == 0 {
					i++
					return iam.Token{
						Value:     "firstToken",
						ExpiresAt: later,
					}, nil
				}
				if i == 1 {
					return iam.Token{
						Value:     "secondToken",
						ExpiresAt: later,
					}, nil
				}
				return iam.Token{}, xerrors.New("shouldn't be possible")
			},
			nowF: func() time.Time { return later },
		}
		gotToken, err := ts.Token(context.Background())
		assert.NoError(t, err)
		assert.Equal(t, "firstToken", gotToken)

		gotToken, err = ts.Token(context.Background())
		assert.NoError(t, err)
		assert.Equal(t, "secondToken", gotToken)
	})

	t.Run("concurrent reads", func(t *testing.T) {
		var run int32
		ts := &cachedToken{
			tokenF: func(context.Context) (iam.Token, error) {
				if run == 0 {
					atomic.AddInt32(&run, 1)
					return iam.Token{
						Value:     "iamToken",
						ExpiresAt: later,
					}, nil
				}
				return iam.Token{}, xerrors.New("should only be called once")
			},
			nowF: func() time.Time { return now },
		}

		wg := sync.WaitGroup{}
		ctx := context.Background()
		goroutines := 100
		start := make(chan interface{})
		for i := 0; i < goroutines; i++ {
			wg.Add(1)
			go func() {
				<-start // so that goroutines start at the same time
				gotToken, err := ts.Token(ctx)
				assert.NoError(t, err)
				assert.Equal(t, "iamToken", gotToken)
				wg.Done()
			}()
		}
		close(start)
		wg.Wait()
	})
}

func BenchmarkToken(b *testing.B) {
	ts := &cachedToken{
		tokenF: func(context.Context) (iam.Token, error) {
			return iam.Token{
				Value:     "iamToken",
				ExpiresAt: later,
			}, nil
		},
		nowF: func() time.Time { return now },
	}
	ctx := context.Background()
	for i := 0; i < b.N; i++ {
		if _, err := ts.Token(ctx); err != nil {
			b.Fatalf("unexpected: %s", err)
		}
	}
}
