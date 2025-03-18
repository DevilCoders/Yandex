package integration

import (
	"bytes"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"go.uber.org/goleak"

	"a.yandex-team.ru/library/go/blockcodecs"
	_ "a.yandex-team.ru/library/go/blockcodecs/all"
	"a.yandex-team.ru/library/go/test/yatest"
)

func TestCodecIDs(t *testing.T) {
	var ids bytes.Buffer

	cpp, err := yatest.BinaryPath("library/go/blockcodecs/integration/cpp/cpp")
	require.NoError(t, err)

	cmd := exec.Command(cpp, "list")
	cmd.Stdout = &ids
	cmd.Stderr = os.Stderr
	require.NoError(t, cmd.Run())

	cppCodecs := map[string]blockcodecs.CodecID{}
	for _, line := range strings.Split(ids.String(), "\n") {
		if line == "" {
			continue
		}

		var name string
		var id blockcodecs.CodecID

		_, err := fmt.Sscanf(line, "%s %d", &name, &id)
		require.NoError(t, err)

		cppCodecs[name] = id
	}

	for _, goCodec := range blockcodecs.ListCodecs() {
		assert.Equal(t, goCodec.ID(), cppCodecs[goCodec.Name()], goCodec.Name())
	}
}

func codeCpp(t *testing.T, op, name string, in []byte) []byte {
	cpp, err := yatest.BinaryPath("library/go/blockcodecs/integration/cpp/cpp")
	require.NoError(t, err)

	var out bytes.Buffer

	cmd := exec.Command(cpp, op, name)
	cmd.Stdin = bytes.NewBuffer(in)
	cmd.Stdout = &out
	cmd.Stderr = os.Stderr
	require.NoError(t, cmd.Run())

	return out.Bytes()
}

func TestStreamCompat(t *testing.T) {
	flatTest := os.Getenv("BLOCKCODECS_FLAT_TEST") == ""

	if !flatTest {
		defer goleak.VerifyNone(t)
	}

	var testInputs = map[string][]byte{
		"empty": nil,
		"small": []byte("c"),
		"text":  []byte("The quick brown fox jumps over the lazy dog"),
		"big":   bytes.Repeat([]byte{1, 2, 3, 4}, 20<<20),
	}

	tRun := func(t *testing.T, name string, cb func(*testing.T)) {
		if flatTest {
			cb(t)
		} else {
			t.Run(name, cb)
		}
	}

	for _, codec := range blockcodecs.ListCodecs() {
		codec := codec
		tRun(t, codec.Name(), func(t *testing.T) {
			if !flatTest {
				t.Parallel()
			}

			for inputName, input := range testInputs {

				tRun(t, inputName+"-cpp2go", func(t *testing.T) {
					coded := codeCpp(t, "encode", codec.Name(), input)

					decoded, err := ioutil.ReadAll(blockcodecs.NewDecoder(bytes.NewBuffer(coded)))
					require.NoError(t, err)

					if len(input) == 0 {
						require.Empty(t, decoded)
					} else {
						require.Equal(t, decoded, input)
					}
				})

				tRun(t, inputName+"-go2cpp", func(t *testing.T) {
					var coded bytes.Buffer

					e := blockcodecs.NewEncoder(&coded, codec)
					_, err := e.Write(input)
					require.NoError(t, err)
					require.NoError(t, e.Close())

					decoded := codeCpp(t, "decode", codec.Name(), coded.Bytes())

					if len(input) == 0 {
						require.Empty(t, decoded)
					} else {
						require.Equal(t, decoded, input)
					}
				})

				tRun(t, inputName+"-go2go", func(t *testing.T) {
					var coded bytes.Buffer

					e := blockcodecs.NewEncoder(&coded, codec)
					_, err := e.Write(input)
					require.NoError(t, err)
					require.NoError(t, e.Close())

					decoded, err := ioutil.ReadAll(blockcodecs.NewDecoder(&coded))
					require.NoError(t, err)

					if len(input) == 0 {
						require.Empty(t, decoded)
					} else {
						require.Equal(t, decoded, input)
					}
				})
			}
		})
	}
}
