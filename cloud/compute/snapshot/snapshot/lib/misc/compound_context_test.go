package misc

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestCompoundContext(t *testing.T) {
	a := assert.New(t)

	c1, cancel1 := context.WithCancel(context.Background())
	c2, cancel2 := context.WithCancel(context.Background())

	type testKey string
	c1 = context.WithValue(c1, testKey("key"), "val1")
	c1 = context.WithValue(c1, testKey("key1"), "val1")

	c2 = context.WithValue(c2, testKey("key"), "val2")
	c2 = context.WithValue(c2, testKey("key2"), "val2")

	c := CompoundContext(c1, c2)

	a.NoError(c.Err())

	a.Equal("val1", c.Value(testKey("key")))
	a.Equal("val1", c.Value(testKey("key1")))
	a.Equal("val2", c.Value(testKey("key2")))

	select {
	case <-c.Done():
		t.Fatal("compound ctx channel is closed")
	default:
	}

	cancel2()

	time.Sleep(10 * time.Millisecond)

	select {
	case <-c.Done():
	default:
		t.Fatal("compound ctx channel is not closed")
	}

	cancel1()

	select {
	case <-c.Done():
	default:
		t.Fatal("compound ctx channel is not closed")
	}

}
