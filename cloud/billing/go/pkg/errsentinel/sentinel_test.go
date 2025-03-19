package errsentinel

import (
	"errors"
	"fmt"
	"io"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestSentinelWrapNil(t *testing.T) {
	sentinel := New("sentinel")
	assert.Panics(t, func() { assert.NoError(t, sentinel.Wrap(nil)) })
}

func TestSentinelWrap(t *testing.T) {
	sentinel := New("sentinel")
	assert.EqualError(t, sentinel.Wrap(New("err")), "sentinel: err")
}

func TestSentinelMultiWrap(t *testing.T) {
	top := New("top")
	middle := New("middle")
	err := top.Wrap(middle.Wrap(New("bottom")))
	assert.EqualError(t, err, "top: middle: bottom")
}

func TestSentinelIs(t *testing.T) {
	sentinel := New("sentinel")
	assert.True(t, errors.Is(sentinel, sentinel))
	assert.True(t, errors.Is(sentinel.Wrap(New("err")), sentinel))
	assert.True(t, errors.Is(New("err").Wrap(sentinel), sentinel))
	assert.True(t, errors.Is(fmt.Errorf("wrapper: %w", sentinel), sentinel))
}

func TestSentinelMultiWrapIs(t *testing.T) {
	top := New("top")
	middle := New("middle")
	err := top.Wrap(middle.Wrap(io.EOF))
	assert.True(t, errors.Is(err, top))
	assert.True(t, errors.Is(err, middle))
	assert.True(t, errors.Is(err, io.EOF))
	assert.False(t, errors.Is(err, New("random")))
}

func TestSentinelAs(t *testing.T) {
	sentinel := New("sentinel")
	var target *Sentinel

	assert.True(t, errors.As(sentinel, &target))
	assert.NotNil(t, target)
	target = nil

	assert.True(t, errors.As(sentinel.Wrap(New("err")), &target))
	assert.NotNil(t, target)
	target = nil

	assert.True(t, errors.As(New("err").Wrap(sentinel), &target))
	assert.NotNil(t, target)
	target = nil

	assert.True(t, errors.As(fmt.Errorf("wrapper: %w", sentinel), &target))
	assert.NotNil(t, target)
	target = nil
}

func TestSentinelMultiWrapAs(t *testing.T) {
	top := New("top")
	middle := New("middle")
	err := top.Wrap(middle.Wrap(io.EOF))

	var target *Sentinel
	assert.True(t, errors.As(err, &target))
	assert.NotNil(t, target)
}

func TestSentinelFormatting(t *testing.T) {
	sentinel := New("sentinel")
	assert.Equal(t, "sentinel", fmt.Sprintf("%s", sentinel))
	assert.Equal(t, "sentinel", fmt.Sprintf("%v", sentinel))
	assert.Equal(t, "sentinel", fmt.Sprintf("%+v", sentinel))
}
