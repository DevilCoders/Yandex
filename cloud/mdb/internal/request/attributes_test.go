package request

import (
	"context"
	"fmt"
	"sync"
	"testing"
)

func TestAttributesConcurrent(t *testing.T) {
	ctx := WithAttributes(context.Background())
	const n = 10
	writeWG := sync.WaitGroup{}
	readLogWG := sync.WaitGroup{}
	readSentryWG := sync.WaitGroup{}
	writeWG.Add(n)
	readLogWG.Add(n)
	readSentryWG.Add(n)
	for i := 0; i < n; i++ {
		go func(ctx context.Context, i int) {
			SetRequestCloudID(ctx, fmt.Sprintf("cloud%d", i))
			SetRequestFolderID(ctx, fmt.Sprintf("folder%d", i))
			SetRequestUserID(ctx, fmt.Sprintf("user_id%d", i))
			SetRequestUserType(ctx, fmt.Sprintf("user_type%d", i))
			writeWG.Done()
		}(ctx, i)
		go func(ctx context.Context, i int) {
			LogFields(ctx)
			readLogWG.Done()
		}(ctx, i)
		go func(ctx context.Context, i int) {
			SentryTags(ctx)
			readSentryWG.Done()
		}(ctx, i)
	}
	writeWG.Wait()
	readLogWG.Wait()
	readSentryWG.Wait()
}
