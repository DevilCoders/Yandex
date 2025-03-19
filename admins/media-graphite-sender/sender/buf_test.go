package sender

import (
	"io/ioutil"
	"os"
	"testing"

	"github.com/stretchr/testify/assert"
)

func init() {
	logger = setupLogger(defaultConfig())
	logTrace = true
}

func TestBuf(t *testing.T) {
	store := newBufStore(1)

	buf := store.getBuffer()
	assert.Equal(t, 0, len(store.queue))
	buf.add([]byte("123456789"))
	assert.Equal(t, 9, buf.maxSize)
	assert.Equal(t, 9, buf.size())
	assert.Equal(t, 1, buf.len())

	buf.add([]byte("0"))
	assert.Equal(t, buf.bytes(), []byte("1234567890"))

	assert.Equal(t, 10, buf.maxSize)
	assert.Equal(t, 10, buf.size())
	assert.Equal(t, 2, buf.len())

	store.returnToQueue(buf)
	assert.Equal(t, 1, len(store.queue))
	buf = store.getBuffer()
	assert.Equal(t, 10, buf.maxSize, "expected allocated size tracking")
	assert.Equal(t, 0, buf.size(), "expected buf reset")
	assert.Equal(t, 0, buf.len(), "expected buf reset")
	buf.add([]byte("fresh"))
	assert.Equal(t, 5, buf.size())
	assert.Equal(t, 10, buf.maxSize)

	assert.Equal(t, 0, len(store.queue))
	buf2 := store.getBuffer()
	assert.Equal(t, 0, buf2.maxSize, "expected new buffer")
	assert.Equal(t, 0, len(store.queue))

	store.returnToQueue(buf)
	store.returnToQueue(buf2)
	assert.Equal(t, 1, len(store.queue))
	buf = store.getBuffer()
	assert.Equal(t, 10, buf.maxSize, "expected allocated size tracking")

	assert.Equal(t, buf.bytes(), []byte{})

}

func TestBufFromFile(t *testing.T) {
	store := newBufStore(1)
	buf := store.getBuffer()
	assert.Equal(t, 0, len(store.queue))
	buf.add([]byte("123456789"))
	assert.Equal(t, 9, buf.size())
	assert.Equal(t, 1, buf.len())

	f, err := ioutil.TempFile(".", "buffer-*.txt")
	if !assert.NoError(t, err) {
		t.FailNow()
	}
	defer os.Remove(f.Name())
	_, err = f.Write(buf.bytes())
	assert.NoError(t, err)
	_ = f.Close()

	buf2, err := store.bufferFromFile(f.Name()) // no newline in buffered data
	assert.NoError(t, err)
	assert.Equal(t, []byte(nil), buf2.bytes())

	buf.add([]byte("\n"))
	assert.NoError(t, ioutil.WriteFile(f.Name(), buf.bytes(), 0644))

	buf2, err = store.bufferFromFile(f.Name()) // no newline in buffered data
	assert.NoError(t, err)
	assert.Equal(t, buf.bytes(), buf2.bytes())

	buf2, err = store.bufferFromFile(f.Name() + "not-exists-here")
	assert.Error(t, err)
	assert.Nil(t, buf2)
}
