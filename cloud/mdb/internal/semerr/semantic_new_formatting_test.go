package semerr

import (
	"testing"

	"a.yandex-team.ru/library/go/core/xerrors/assertxerrors"
)

func TestNewFormatting(t *testing.T) {
	constructor := func(t *testing.T) error {
		return NotFound("not found")
	}
	expected := assertxerrors.Expectations{
		ExpectedS: "not found",
		ExpectedV: "not found",
		Frames: assertxerrors.NewStackTraceModeExpectation(`
not found
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestNewFormatting.func1
        cloud/mdb/internal/semerr/semantic_new_formatting_test.go:11
`,
		),
		Stacks: assertxerrors.NewStackTraceModeExpectation(`
not found
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestNewFormatting.func1
        cloud/mdb/internal/semerr/semantic_new_formatting_test.go:11
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
`,
			3, 4, 5, 6,
		),
		StackThenFrames: assertxerrors.NewStackTraceModeExpectation(`
not found
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestNewFormatting.func1
        cloud/mdb/internal/semerr/semantic_new_formatting_test.go:11
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
`,
			3, 4, 5, 6,
		),
		StackThenNothing: assertxerrors.NewStackTraceModeExpectation(`
not found
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestNewFormatting.func1
        cloud/mdb/internal/semerr/semantic_new_formatting_test.go:11
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
`,
			3, 4, 5, 6,
		),
		Nothing: assertxerrors.NewStackTraceModeExpectation("not found"),
	}
	assertxerrors.RunTestsPerMode(t, expected, constructor)
}
