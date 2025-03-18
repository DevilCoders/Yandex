package yamake

import (
	"bytes"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestFilterEmptyIfs(t *testing.T) {
	ifs := []Macro{
		{Name: "IF", Args: []string{"OS_LINUX"}},
		{Name: "ENDIF"},

		{Name: "IF", Args: []string{"OS_DARWIN"}},
		{Name: "ELSEIF"},
		{Name: "ELSE"},
		{Name: "ENDIF"},

		{Name: "IF", Args: []string{"OS_WINDOWS"}},
		{Name: "FOO"},
		{Name: "ENDIF"},

		{Name: "BAR"},
	}

	expected := []Macro{
		{Name: "IF", Args: []string{"OS_WINDOWS"}},
		{Name: "FOO"},
		{Name: "ENDIF"},

		{Name: "BAR"},
	}

	assert.Equal(t, expected, RemoveEmptyIfs(ifs))
}

var testYaMake0 = `
GO_LIBRARY()

OWNER(prime)

LICENSE(GPL)

SRCS(
    foo.c
)

SRCS(
    a.go
    b.go
)

IF (OS_LINUX)
    SRCS(
        a_linux.go
    )
ENDIF()

IF (OS_DARWIN)
    SRCS(
        a_darwin.go
    )
ENDIF()

IF (
	OS_DARWIN
	AND
	ARCH_ARM64
)
    SRCS(
        a_darwin_arm64.go
    )
ENDIF()

IF (OS_WINDOWS)
    SRCS(
        a_windows.go
    )
ENDIF()

END()

RECURSE(
    cpp
)

RECURSE(
    multierr
    some
)

RECURSE_FOR_TESTS(gotest)

IF (OS_DARWIN)
    RECURSE(darwin)
ENDIF()

IF (
	OS_DARWIN
	AND
	ARCH_ARM64
)
    RECURSE(darwin_arm64)
ENDIF()

IF (
	OS_DARWIN
	AND
	ARCH_X86_64
)
    RECURSE(darwin_amd64)
ENDIF()

IF (OS_WINDOWS)
    RECURSE(windows)
ENDIF()

IF (OS_LINUX)
    FOO()

    RECURSE(unix)
ENDIF()

IF (ARCH_X86_64)
    RECURSE(amd64)
ENDIF()

IF (OS_LINUX || OS_DARWIN)
    RECURSE(custom)
ENDIF()
`

func TestLoad(t *testing.T) {
	buf := bytes.NewBufferString(testYaMake0)
	yaMake, err := ReadYaMake(buf)
	require.NoError(t, err)

	assert.Equal(t, "GO_LIBRARY", yaMake.Module.Name)
	assert.Equal(t, []string{"cpp", "multierr", "some"}, yaMake.Recurse)
	assert.Equal(t, []string{"gotest"}, yaMake.RecurseForTests)
	assert.Equal(t, []string{"prime"}, yaMake.Owner)
	assert.Equal(t, []string{"GPL"}, yaMake.License)

	assert.Empty(t, yaMake.Middle)

	assert.Equal(t, yaMake.TargetRecurse[Linux], []string{"unix"})
	assert.Equal(t, yaMake.TargetRecurse[Darwin], []string{"darwin"})
	assert.Equal(t, yaMake.TargetRecurse[DarwinARM64], []string{"darwin_arm64"})
	assert.Equal(t, yaMake.TargetRecurse[DarwinAMD64], []string{"darwin_amd64"})
	assert.Equal(t, yaMake.TargetRecurse[Windows], []string{"windows"})
	assert.Equal(t, yaMake.TargetRecurse[AMD64], []string{"amd64"})

	assert.Equal(t, yaMake.CustomConditionRecurse, map[string]struct{}{"custom": struct{}{}})

	expectedSuffix := []Macro{
		{Name: "IF", Args: []string{"OS_LINUX"}},
		{Name: "FOO"},
		{Name: "ENDIF"},
		{Name: "IF", Args: []string{"OS_LINUX", "||", "OS_DARWIN"}},
		{Name: "RECURSE", Args: []string{"custom"}},
		{Name: "ENDIF"},
	}

	assert.Equal(t, expectedSuffix, yaMake.Suffix)
}

var testYaMake1 = `
GO_LIBRARY()

OWNER(prime)

IF (FOO)
    FOO()
ELSEIF(BAR)
    BAR()
ELSE()
    ZOG()
ENDIF()

RESOURCE(
    foo.txt bar.txt
)

SRCS(a.go)

GO_TEST_SRCS(a_test.go)

GO_XTEST_SRCS(b_test.go)

IF (ARCH_X86_64)
    SRCS(stub_arm64.go)
ENDIF()

IF (OS_LINUX)
    SRCS(
        inotify_linux.go
        epoll_linux.go
    )

    GO_TEST_SRCS(inotify_test.go)
ENDIF()

IF (
    OS_LINUX
    AND
    ARCH_ARM64
)
    SRCS(inotify_linux_arm64.go)

    GO_TEST_SRCS(inotify_linux_arm64_test.go)
ENDIF()

END()

RECURSE(
    foo
    bar
    # zog
)

RECURSE_FOR_TESTS(gotest)

IF (OS_LINUX)
    RECURSE(unix)
ENDIF()
`[1:]

func TestSave(t *testing.T) {
	yaMake := NewYaMake()
	yaMake.Module.Name = "GO_LIBRARY"
	yaMake.Owner = []string{"prime"}
	yaMake.Recurse = []string{"foo", "bar", "# zog"}
	yaMake.RecurseForTests = []string{"gotest"}
	yaMake.Middle = []Macro{
		{Name: "IF", Args: []string{"FOO"}},
		{Name: "FOO"},
		{Name: "ELSEIF", Args: []string{"BAR"}},
		{Name: "BAR"},
		{Name: "ELSE"},
		{Name: "ZOG"},
		{Name: "ENDIF"},
		{Name: "RESOURCE", Args: []string{"foo.txt", "bar.txt"}},
	}

	yaMake.TargetRecurse[Linux] = []string{"unix"}

	yaMake.CommonSources = Sources{
		Files:        []string{"a.go"},
		TestGoFiles:  []string{"a_test.go"},
		XTestGoFiles: []string{"b_test.go"},
	}

	yaMake.TargetSources[AMD64] = &Sources{
		Files: []string{"stub_arm64.go"},
	}

	yaMake.TargetSources[Linux] = &Sources{
		Files:       []string{"inotify_linux.go", "epoll_linux.go"},
		TestGoFiles: []string{"inotify_test.go"},
	}

	yaMake.TargetSources[LinuxARM64] = &Sources{
		Files:       []string{"inotify_linux_arm64.go"},
		TestGoFiles: []string{"inotify_linux_arm64_test.go"},
	}

	var out bytes.Buffer
	yaMake.WriteResult(&out)

	assert.Equal(t, testYaMake1, out.String())
}

var testYaMake2 = `
GO_LIBRARY()

#CGO_SRCS(
#    c.go
#)
# DISABLED
#GO_XTEST_SRCS(
#    c_test.go
#)

SRCS(
    a.go
    b.go
    # d.go
)

END()
`[1:]

func TestComments(t *testing.T) {
	buf := bytes.NewBufferString(testYaMake2)
	yaMake, err := ReadYaMake(buf)
	require.NoError(t, err)
	require.Equal(t, map[string]struct{}{"d.go": {}}, yaMake.DisabledFiles)

	yaMake.CommonSources = Sources{
		Files: []string{"a.go", "b.go", "d.go"},
	}
	yaMake.DisableFiles()

	var out bytes.Buffer
	yaMake.WriteResult(&out)

	assert.Equal(t, testYaMake2, out.String())
}

var protoYaMake = `
PROTO_LIBRARY()

OWNER(prime)

SRCS(msg.proto)

END()
`[1:]

func TestProtoLibrary(t *testing.T) {
	buf := bytes.NewBufferString(protoYaMake)
	yaMake, err := ReadYaMake(buf)
	require.NoError(t, err)

	var out bytes.Buffer
	yaMake.WriteResult(&out)

	assert.Equal(t, protoYaMake, out.String())
}

var pyProgramYaMake = `
PY2_PROGRAM()

PEERDIR(
    contrib/python/requests
    yt/python/yt/wrapper
)

PY_SRCS(__main__.py)

END()
`[1:]

func TestPyProgramEdit(t *testing.T) {
	buf := bytes.NewBufferString(pyProgramYaMake)
	yaMake, err := ReadYaMake(buf)
	require.NoError(t, err)

	var out bytes.Buffer
	yaMake.WriteResult(&out)

	assert.Equal(t, pyProgramYaMake, out.String())
}

var goProgramLicense1YaMake = `
GO_PROGRAM()

LICENSE(Lic1 Lic2)

END()
`[1:]

var goProgramLicense1YaMakeFormatted = `
GO_PROGRAM()

LICENSE(
    Lic1 AND
    Lic2
)

END()
`[1:]

var goProgramLicense2YaMake = `
GO_PROGRAM()

LICENSE(Lic1 OR Lic2)

END()
`[1:]

var goProgramLicense2YaMakeFormatted = `
GO_PROGRAM()

LICENSE(
    Lic1 OR
    Lic2
)

END()
`[1:]

var goProgramLicense3YaMake = `
GO_PROGRAM()

LICENSE(Lic1 OR Lic2 AND Lic3)

END()
`[1:]

var goProgramLicense3YaMakeFormatted = `
GO_PROGRAM()

LICENSE(
    Lic1 OR
    Lic2 AND
    Lic3
)

END()
`[1:]

var goProgramLicense4YaMake = `
GO_PROGRAM()

LICENSE(Lic1 Lic2 OR Lic3)

END()
`[1:]

var goProgramLicense4YaMakeFormatted = `
GO_PROGRAM()

LICENSE(
    Lic1 AND
    Lic2 OR
    Lic3
)

END()
`[1:]

var goProgramLicense5YaMake = `
GO_PROGRAM()

LICENSE(Lic1 WITH exception Lic2 Lic3)

END()
`[1:]

var goProgramLicense5YaMakeFormatted = `
GO_PROGRAM()

LICENSE(
    Lic1 WITH exception AND
    Lic2 AND
    Lic3
)

END()
`[1:]

func TestLicenses(t *testing.T) {
	buf := bytes.NewBufferString(goProgramLicense1YaMake)
	yaMake, err := ReadYaMake(buf)
	require.NoError(t, err)

	assert.Equal(t, []string{"Lic1", "Lic2"}, yaMake.License)

	var out bytes.Buffer
	yaMake.WriteResult(&out)

	assert.Equal(t, goProgramLicense1YaMakeFormatted, out.String())

	// goProgramLicense2
	buf = bytes.NewBufferString(goProgramLicense2YaMake)
	yaMake, err = ReadYaMake(buf)
	require.NoError(t, err)

	assert.Equal(t, []string{"Lic1", "OR", "Lic2"}, yaMake.License)

	out.Reset()
	yaMake.WriteResult(&out)

	assert.Equal(t, goProgramLicense2YaMakeFormatted, out.String())

	// goProgramLicense3
	buf = bytes.NewBufferString(goProgramLicense3YaMake)
	yaMake, err = ReadYaMake(buf)
	require.NoError(t, err)

	assert.Equal(t, []string{"Lic1", "OR", "Lic2", "AND", "Lic3"}, yaMake.License)

	out.Reset()
	yaMake.WriteResult(&out)

	assert.Equal(t, goProgramLicense3YaMakeFormatted, out.String())

	// goProgramLicense4
	buf = bytes.NewBufferString(goProgramLicense4YaMake)
	yaMake, err = ReadYaMake(buf)
	require.NoError(t, err)

	assert.Equal(t, []string{"Lic1", "Lic2", "OR", "Lic3"}, yaMake.License)

	out.Reset()
	yaMake.WriteResult(&out)

	assert.Equal(t, goProgramLicense4YaMakeFormatted, out.String())

	// goProgramLicense5
	buf = bytes.NewBufferString(goProgramLicense5YaMake)
	yaMake, err = ReadYaMake(buf)
	require.NoError(t, err)

	assert.Equal(t, []string{"Lic1", "WITH", "exception", "Lic2", "Lic3"}, yaMake.License)

	out.Reset()
	yaMake.WriteResult(&out)

	assert.Equal(t, goProgramLicense5YaMakeFormatted, out.String())
}

var ignoreYaMake = `
# yo ignore:file

GO_PROGRAM()

END()
`[1:]

func TestIgnoreFile(t *testing.T) {
	buf := bytes.NewBufferString(ignoreYaMake)
	yaMake, err := ReadYaMake(buf)
	require.NoError(t, err)

	require.True(t, yaMake.IsMarkedIgnore())
}
