package testdb

import (
	"context"
	"testing"
	"time"
)

func TestDB(t *testing.T) {
	db := DB()
	ctx, cancel := context.WithTimeout(context.Background(), time.Microsecond*100)
	defer cancel()
	_ = db.PingContext(ctx)
}
