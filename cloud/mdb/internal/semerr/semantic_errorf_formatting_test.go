package semerr

import (
	"testing"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/core/xerrors/assertxerrors"
)

func TestErrorfFormatting(t *testing.T) {
	constructor := func(t *testing.T) error {
		err := NotFound("not found")
		return xerrors.Errorf("text: %w", err)
	}
	expected := assertxerrors.Expectations{
		ExpectedS: "text: not found",
		ExpectedV: "text: not found",
		Frames: assertxerrors.NewStackTraceModeExpectation(`
text:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestErrorfFormatting.func1
        cloud/mdb/internal/semerr/semantic_errorf_formatting_test.go:13
not found
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestErrorfFormatting.func1
        cloud/mdb/internal/semerr/semantic_errorf_formatting_test.go:12
`,
		),
		Stacks: assertxerrors.NewStackTraceModeExpectation(`
text:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestErrorfFormatting.func1
        cloud/mdb/internal/semerr/semantic_errorf_formatting_test.go:13
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
not found
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestErrorfFormatting.func1
        cloud/mdb/internal/semerr/semantic_errorf_formatting_test.go:12
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
`,
			3, 4, 5, 6, 10, 11, 12, 13,
		),
		StackThenFrames: assertxerrors.NewStackTraceModeExpectation(`
text:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestErrorfFormatting.func1
        cloud/mdb/internal/semerr/semantic_errorf_formatting_test.go:13
not found
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestErrorfFormatting.func1
        cloud/mdb/internal/semerr/semantic_errorf_formatting_test.go:12
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
`,
			6, 7, 8, 9,
		),
		StackThenNothing: assertxerrors.NewStackTraceModeExpectation(`
text: not found
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestErrorfFormatting.func1
        cloud/mdb/internal/semerr/semantic_errorf_formatting_test.go:12
    a.yandex-team.ru/library/go/core/xerrors/assertxerrors.RunTestsPerMode.func1
        /home/sidh/devel/go/src/a.yandex-team.ru/library/go/core/xerrors/assertxerrors/assertxerrors.go:18
    testing.tRunner
        /home/sidh/.ya/tools/v4/774223543/src/testing/testing.go:1127
`,
			3, 4, 5, 6,
		),
		Nothing: assertxerrors.NewStackTraceModeExpectation("text: not found"),
	}
	assertxerrors.RunTestsPerMode(t, expected, constructor)
}
