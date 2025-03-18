package main

import (
	"bytes"
	"go/format"
	"io/ioutil"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"testing"

	"github.com/google/go-cmp/cmp"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/test/go_toolchain/gotoolchain"
	"a.yandex-team.ru/library/go/test/yatest"
)

var testedResolvers = []string{
	"adaptive",
	"golist",
	"gopackages",
}

func init() {
	if err := gotoolchain.Setup(os.Setenv); err != nil {
		panic(err)
	}

	gopath := yatest.WorkPath("fakehome/go")
	if err := os.MkdirAll(gopath, 0777); err != nil {
		panic(err)
	}

	_ = os.Setenv("HOME", filepath.Dir(gopath))
	_ = os.Setenv("GOPATH", gopath)
}

func Test_unformatted(t *testing.T) {
	wd, err := os.Getwd()
	require.NoError(t, err)
	defer func() {
		_ = os.Chdir(wd)
	}()

	err = os.Chdir("testdata/unformatted")
	require.NoError(t, err)

	matches, err := filepath.Glob("*.go")
	require.NoError(t, err)
	require.NotEmpty(t, matches)

	for _, match := range matches {
		t.Run(match, func(t *testing.T) {
			filename := filepath.Base(match)
			referenceFile := "../formatted/" + filename

			expected, err := ioutil.ReadFile(referenceFile)
			require.NoError(t, err)

			for _, r := range testedResolvers {
				t.Run(r, func(t *testing.T) {
					err := os.Setenv("YOIMPORTS_RESOLVER", r)
					require.NoError(t, err)

					importsResolver = newImportsResolver()
					t.Run("processFile", func(t *testing.T) {
						actual, err := processFile(filepath.Base(match))
						require.NoError(t, err)

						require.Equal(t, string(expected), string(actual))
					})

					t.Run("processStream", func(t *testing.T) {
						original, err := os.Open(match)
						require.NoError(t, err)
						defer func() {
							_ = original.Close()
						}()

						var actual bytes.Buffer
						err = processStream(&actual, original)
						require.NoError(t, err)

						require.Equal(t, string(expected), actual.String())
					})
				})
			}
		})
	}
}

func Test_processFileWithVendored(t *testing.T) {
	err := filepath.Walk("testdata/", func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		if info.IsDir() && info.Name() == "cheburek" {
			return filepath.SkipDir
		}

		if !isGoFile(info) {
			return nil
		}

		t.Run(path, func(t *testing.T) {
			originalSource, err := ioutil.ReadFile(path)
			require.NoError(t, err)

			source, err := processFile(path)
			assert.NoError(t, err)

			if isVendored(path) ||
				strings.Contains(path, "/formatted/") {
				assert.Nil(t, source, cmp.Diff(originalSource, source))
			}

			if strings.Contains(path, "/unformatted/") {
				assert.NotNil(t, source)
			}
		})

		return nil
	})
	require.NoError(t, err)
}

func Test_processFileGenerated(t *testing.T) {
	matches, err := filepath.Glob("testdata/gen/*.go")
	require.NoError(t, err)
	require.NotEmpty(t, matches)

	for _, match := range matches {
		t.Run(match, func(t *testing.T) {
			actual, err := processFile(match)
			assert.NoError(t, err)
			assert.Nil(t, actual)
		})
	}
}

func Test_isVendored(t *testing.T) {
	testCases := []struct {
		path     string
		expected bool
	}{
		{"testdata/formatted/01.go", false},
		{"testdata/unformatted/01.go", false},
		{"testdata/vendor/bb.yandex-team.ru/go/somerepo/main.go", true},
		{"vendor/github.com/gofrs/uuid/uuid.go", true},
	}

	for _, tc := range testCases {
		t.Run(tc.path, func(t *testing.T) {
			// make path platform-specific
			path := filepath.Join(strings.Split(tc.path, "/")...)
			assert.Equal(t, tc.expected, isVendored(path))
		})
	}
}

func Test_std(t *testing.T) {
	cases := []struct {
		source   string
		expected string
	}{
		{
			source:   "package kek\nconst a = 1\n\t\nconst b = 2\n",
			expected: "package kek\n\nconst a = 1\n\nconst b = 2\n",
		},
		{
			source:   "package kek\nfunc a() {\n1\n\t\n2\n}\n",
			expected: "package kek\n\nfunc a() {\n\t1\n\n\t2\n}\n",
		},
		{
			source:   "package kek",
			expected: "package kek\n",
		},
		{
			source:   "package kek\nimport _ \"lol\"",
			expected: "package kek\n\nimport _ \"lol\"\n",
		},
	}

	for i, tc := range cases {
		t.Run("hand_"+strconv.Itoa(i), func(t *testing.T) {
			formatted, err := processBytes([]byte(tc.source))
			require.NoError(t, err)
			require.Equal(t, tc.expected, string(formatted))
		})
	}

	for i, tc := range cases {
		t.Run("std_"+strconv.Itoa(i), func(t *testing.T) {
			formatted, err := processBytes([]byte(tc.source))
			require.NoError(t, err)
			expected, err := format.Source([]byte(tc.source))
			require.NoError(t, err)
			require.Equal(t, expected, formatted)
		})
	}
}
