package protoseq

import (
	"bytes"
	"encoding/binary"
	"strings"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/x/encoding/protoseq/internal"
)

func TestEncoderDecoder(t *testing.T) {
	const noiters = 2
	const value = "imperator@"

	var buf = new(bytes.Buffer)
	var msg = &internal.Hail{Whom: value}

	var enc = NewEncoder(buf)
	for i := 0; i != noiters; i++ {
		if err := enc.Encode(msg); err != nil {
			t.Fatalf("failed to encode: %s", err)
		}
	}

	var dec = NewDecoder(buf)
	var cnt = 0
	for dec.More() {
		var msg internal.Hail
		if err := dec.Decode(&msg); err != nil {
			t.Fatalf("failed to decode: %s", err)
		}
		if msg.Whom != value {
			t.Fatalf("wrong value decoded: `%s`", msg.Whom)
		}
		cnt++
	}

	if cnt != noiters {
		t.Errorf("mismatch number of encoded and decoded messages: %d != %d",
			noiters, cnt)
	}

	require.NoError(t, dec.Err())
}

func TestDecodeStates(t *testing.T) {
	t.Run("DecodeToDecode", func(t *testing.T) {
		var buf = new(bytes.Buffer)
		var dec = NewDecoder(buf)
		var enc = NewEncoder(buf)
		var msg = &internal.Hail{Whom: "to the king, baby"}

		if err := enc.Encode(msg); err != nil {
			t.Fatalf("failed to encode: %s", err)
		}

		if !dec.More() {
			t.Fatalf("failed to read frame size: %s", dec.Err())
		}

		for i := 0; i != 2; i++ {
			var out internal.Hail
			if err := dec.Decode(&out); err != nil {
				t.Fatalf("failed to decode (%d): %s", i, err)
			}
		}

		require.NoError(t, dec.Err())
	})
}

func TestDecodeErrors(t *testing.T) {
	buf := new(bytes.Buffer)
	enc := NewEncoder(buf)
	msg := internal.Hail{Whom: "to the king, baby"}

	err := enc.Encode(&msg)
	require.NoError(t, err, "failed to encode internal.Hail")

	err = enc.Encode(&msg)
	require.NoError(t, err, "failed to encode internal.Hail")

	dec := NewDecoder(buf)
	ok := dec.More()
	require.Truef(t, ok, "failed to read frame size: %v", dec.Err())

	var out internal.NotHail
	err = dec.Decode(&out)
	require.Error(t, err, "decode unexpected proto msg must fail")
	require.NoError(t, dec.Err(), "decoding errors should not affect iterator")
	require.True(t, dec.More(), "decoding errors should not affect iterator")
}

func TestDecodeOOM(t *testing.T) {
	data := make([]byte, 4)
	binary.LittleEndian.PutUint32(data, ^uint32(0))

	for i := 0; i < 1024; i++ {
		dec := NewDecoder(bytes.NewReader(data))
		require.False(t, dec.More())
	}
}

func benchDecoder(b *testing.B, msgs int) {
	gen := func() []byte {
		var buf bytes.Buffer
		enc := NewEncoder(&buf)
		smallMsg := internal.Hail{Whom: strings.Repeat("A", 128)}
		hugeMsg := internal.Hail{Whom: strings.Repeat("A", 1024)}
		for i := 0; i < msgs; i++ {
			var err error
			if i%2 == 0 {
				err = enc.Encode(&hugeMsg)
			} else {
				err = enc.Encode(&smallMsg)
			}
			require.NoError(b, err)
		}
		return buf.Bytes()
	}

	data := gen()
	b.ResetTimer()
	var msg internal.Hail
	for i := 0; i < b.N; i++ {
		dec := NewDecoder(bytes.NewReader(data))
		for dec.More() {
			_ = dec.Decode(&msg)
		}
	}
}

func BenchmarkDecoder8(b *testing.B) {
	benchDecoder(b, 8)
}

func BenchmarkDecoder128(b *testing.B) {
	benchDecoder(b, 128)
}

func BenchmarkDecoder1024(b *testing.B) {
	benchDecoder(b, 1024)
}
