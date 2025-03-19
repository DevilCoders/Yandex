package semerr

import (
	"io"
	"testing"

	"a.yandex-team.ru/library/go/core/xerrors/assertxerrors"
)

func TestMultiWrapFormatting(t *testing.T) {
	constructor := func(t *testing.T) error {
		err := io.EOF
		err = WrapWithUnavailable(err, "unavailable")
		err = WrapWithNotImplemented(err, "not implemented")
		return WrapWithNotFound(err, "not found")
	}
	expected := assertxerrors.Expectations{
		ExpectedS: "not found: not implemented: unavailable: EOF",
		ExpectedV: "not found: not implemented: unavailable: EOF",
		Frames: assertxerrors.NewStackTraceModeExpectation(`
not found:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:15
not implemented:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:14
unavailable:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:13
EOF`,
		),
		Stacks: assertxerrors.NewStackTraceModeExpectation(`
not found:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:15
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
not implemented:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:14
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
unavailable:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:13
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
EOF`,
			3, 4, 5, 6, 10, 11, 12, 13, 17, 18, 19, 20,
		),
		StackThenFrames: assertxerrors.NewStackTraceModeExpectation(`
not found:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:15
not implemented:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:14
unavailable:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:13
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
EOF`,
			9, 10, 11, 12,
		),
		StackThenNothing: assertxerrors.NewStackTraceModeExpectation(`
not found: not implemented: unavailable:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting.func1
        cloud/mdb/internal/semerr/semantic_multiwrap_formatting_test.go:13
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
EOF`,
			3, 4, 5, 6,
		),
		Nothing: assertxerrors.NewStackTraceModeExpectation("not found: not implemented: unavailable: EOF"),
	}
	assertxerrors.RunTestsPerMode(t, expected, constructor)
}
