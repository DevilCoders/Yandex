package govendor

import (
	"testing"

	"github.com/stretchr/testify/require"
	"golang.org/x/mod/module"
)

var exampleModulesTXT = `
# github.com/spf13/pflag v1.0.3
## explicit
github.com/spf13/pflag
# github.com/stretchr/testify v1.5.1
## explicit
github.com/stretchr/testify/assert
github.com/stretchr/testify/require
# go.uber.org/atomic v1.5.0
## explicit
go.uber.org/atomic
# golang.org/x/crypto v0.0.0-20200323165209-0ec3e9974c59
## explicit
golang.org/x/crypto/pbkdf2
# golang.org/x/lint v0.0.0-20190930215403-16217165b5de
## explicit
golang.org/x/lint
golang.org/x/lint/golint
# golang.org/x/net v0.0.0-20190813141303-74dc4d7220e7
## explicit
golang.org/x/net/html
golang.org/x/net/html/atom
golang.org/x/net/publicsuffix
# golang.org/x/sync v0.0.0-20190423024810-112230192c58
## explicit; go 1.17
golang.org/x/sync/errgroup
golang.org/x/sync/singleflight
# golang.org/x/sys v0.0.0-20200409092240-59c9f1ba88fa
## explicit
golang.org/x/sys/unix
# golang.org/x/text v0.0.0-20170915032832-14c0d48ead0c
## explicit
golang.org/x/text/internal/tag
golang.org/x/text/language
# gopkg.in/yaml.v2 v2.2.0 => gopkg.in/yaml.v2 v2.2.8
## explicit
gopkg.in/yaml.v2
# rsc.io/quote v1.5.2 => rsc.io/quote v1.5.1
## explicit
rsc.io/quote
# rsc.io/sampler v1.3.0
## explicit
rsc.io/sampler
## replaces below
# gopkg.in/yaml.v2 => gopkg.in/yaml.v2 v2.2.8
`[1:]

func TestModulesTXT(t *testing.T) {
	txt, err := ReadModulesTXT([]byte(exampleModulesTXT))
	require.NoError(t, err)

	replaces := map[string]Replace{
		"gopkg.in/yaml.v2": {
			Explicit: module.Version{
				Path:    "gopkg.in/yaml.v2",
				Version: "v2.2.0",
			},
			Old: module.Version{
				Path:    "gopkg.in/yaml.v2",
				Version: "",
			},
			New: module.Version{
				Path:    "gopkg.in/yaml.v2",
				Version: "v2.2.8",
			},
		},
		"rsc.io/quote": {
			Explicit: module.Version{
				Path:    "rsc.io/quote",
				Version: "v1.5.2",
			},
			Old: module.Version{
				Path:    "rsc.io/quote",
				Version: "v1.5.2",
			},
			New: module.Version{
				Path:    "rsc.io/quote",
				Version: "v1.5.1",
			},
		},
	}

	out, err := txt.Encode(replaces)
	require.NoError(t, err)
	require.Equal(t, exampleModulesTXT, string(out))
}
