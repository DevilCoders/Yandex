package yolint

import (
	"github.com/jingyugao/rowserrcheck/passes/rowserr"
	sqlclosecheck "github.com/ryanrolds/sqlclosecheck/pkg/analyzer"
	"github.com/timakin/bodyclose/passes/bodyclose"
	"golang.org/x/tools/go/analysis"
	"golang.org/x/tools/go/analysis/passes/asmdecl"
	"golang.org/x/tools/go/analysis/passes/assign"
	"golang.org/x/tools/go/analysis/passes/atomic"
	"golang.org/x/tools/go/analysis/passes/atomicalign"
	"golang.org/x/tools/go/analysis/passes/bools"
	"golang.org/x/tools/go/analysis/passes/buildtag"
	"golang.org/x/tools/go/analysis/passes/composite"
	"golang.org/x/tools/go/analysis/passes/copylock"
	"golang.org/x/tools/go/analysis/passes/deepequalerrors"
	"golang.org/x/tools/go/analysis/passes/errorsas"
	"golang.org/x/tools/go/analysis/passes/httpresponse"
	"golang.org/x/tools/go/analysis/passes/ifaceassert"
	"golang.org/x/tools/go/analysis/passes/loopclosure"
	"golang.org/x/tools/go/analysis/passes/lostcancel"
	"golang.org/x/tools/go/analysis/passes/nilfunc"
	"golang.org/x/tools/go/analysis/passes/nilness"
	"golang.org/x/tools/go/analysis/passes/shift"
	"golang.org/x/tools/go/analysis/passes/stdmethods"
	"golang.org/x/tools/go/analysis/passes/stringintconv"
	"golang.org/x/tools/go/analysis/passes/structtag"
	"golang.org/x/tools/go/analysis/passes/tests"
	"golang.org/x/tools/go/analysis/passes/unmarshal"
	"golang.org/x/tools/go/analysis/passes/unreachable"
	"golang.org/x/tools/go/analysis/passes/unsafeptr"
	"golang.org/x/tools/go/analysis/passes/unusedresult"
	"golang.org/x/tools/go/analysis/passes/unusedwrite"
	"honnef.co/go/tools/simple"
	"honnef.co/go/tools/staticcheck"
	"honnef.co/go/tools/stylecheck"

	"a.yandex-team.ru/library/go/yolint/internal/middlewares"
	"a.yandex-team.ru/library/go/yolint/internal/passes/copyproto"
	"a.yandex-team.ru/library/go/yolint/internal/passes/deepequalproto"
	"a.yandex-team.ru/library/go/yolint/internal/passes/errcheck"
	"a.yandex-team.ru/library/go/yolint/internal/passes/exhaustivestruct"
	"a.yandex-team.ru/library/go/yolint/internal/passes/importcheck"
	"a.yandex-team.ru/library/go/yolint/internal/passes/printf"
	"a.yandex-team.ru/library/go/yolint/internal/passes/protonaming"
	"a.yandex-team.ru/library/go/yolint/internal/passes/scopelint"
	"a.yandex-team.ru/library/go/yolint/internal/passes/structtagcase"
	"a.yandex-team.ru/library/go/yolint/internal/passes/testifycheck"
	"a.yandex-team.ru/library/go/yolint/internal/passes/ytcheck"
)

// optionalAnalyzers are disabled by default but available via scopelint
var optionalAnalyzers = map[string]struct{}{
	"SA1019":           {},
	"SA4005":           {},
	"SA6002":           {},
	"ST1000":           {},
	"ST1020":           {},
	"ST1021":           {},
	"ST1022":           {},
	"bodyclose":        {},
	"copyproto":        {},
	"exhaustivestruct": {},
	"importcheck":      {},
	"rowserrcheck":     {},
	"sqlclosecheck":    {},
	"structtagcase":    {},
	"ytcheck":          {},
}

func CollectAnalyzers() []*analysis.Analyzer {
	analyzers := collectAllAnalyzers()

	defaultAnalyzers := []*analysis.Analyzer{scopelint.Analyzer}
	for _, analyzer := range analyzers {
		if _, ok := optionalAnalyzers[analyzer.Name]; ok {
			// register optional analyzer in scopelint
			scopelint.Register(analyzer)
			continue
		}
		defaultAnalyzers = append(defaultAnalyzers, analyzer)
	}

	return defaultAnalyzers
}

// collectAllAnalyzers returns all available analyzers
func collectAllAnalyzers() []*analysis.Analyzer {
	analyzers := []*analysis.Analyzer{
		// default vet tool analyzers

		// report mismatches between assembly files and Go declarations
		asmdecl.Analyzer,
		// check for useless assignments
		assign.Analyzer,
		// check for common mistakes using the sync/atomic package
		atomic.Analyzer,
		// checks for non-64-bit-aligned arguments to sync/atomic functions
		atomicalign.Analyzer,
		// check for common mistakes involving boolean operators
		bools.Analyzer,
		// check that +build tags are well-formed and correctly located
		buildtag.Analyzer,
		// detect some violations of the cgo pointer passing rules. Not working in Arcadia for now
		// cgocall.Analyzer,
		// check for unkeyed composite literals
		composite.Analyzer,
		// check for locks erroneously passed by value
		copylock.Analyzer,
		// check for the use of reflect.DeepEqual with error values
		deepequalerrors.Analyzer,
		// check that the second argument to errors.As is a pointer to a type implementing error
		errorsas.Analyzer,
		// check for mistakes using HTTP responses
		httpresponse.Analyzer,
		// check references to loop variables from within nested functions
		loopclosure.Analyzer,
		// check cancel func returned by context.WithCancel is called
		lostcancel.Analyzer,
		// check for useless comparisons between functions and nil
		nilfunc.Analyzer,
		// inspects the control-flow graph of an SSA function and reports errors such as nil pointer dereferences and degenerate nil pointer comparisons
		middlewares.Nogen(nilness.Analyzer),
		// check consistency of Printf format strings and arguments
		printf.Analyzer,
		// check for possible unintended shadowing of variables EXPERIMENTAL
		// shadow.Analyzer,
		// check for shifts that equal or exceed the width of the integer
		shift.Analyzer,
		// check signature of methods of well-known interfaces
		stdmethods.Analyzer,
		// check that struct field tags conform to reflect.StructTag.Get
		structtag.Analyzer,
		// check for common mistaken usages of tests and examples
		middlewares.Nolint(tests.Analyzer),
		// report passing non-pointer or non-interface values to unmarshal
		unmarshal.Analyzer,
		// check for unreachable code
		unreachable.Analyzer,
		// check for invalid conversions of uintptr to unsafe.Pointer
		unsafeptr.Analyzer,
		// check for unused results of calls to some functions
		unusedresult.Analyzer,
		// check for unused writes
		middlewares.Migration(unusedwrite.Analyzer),
		// check for string(int) conversions
		stringintconv.Analyzer,
		// check for impossible interface-to-interface type assertions
		ifaceassert.Analyzer,
		// custom internal analyzers

		// checks all struct fields are filled
		exhaustivestruct.Analyzer,
		// checks errors are not silently ignored
		errcheck.Analyzer,
		// checks proper imports formatting
		middlewares.Migration(importcheck.Analyzer),

		// migration analyzer is here only to register its config
		middlewares.MigrationAnalyzer,

		// check common errors when using testify library
		testifycheck.Analyzer,

		// check protobuf packages are given meaningful names
		middlewares.Migration(protonaming.Analyzer),

		// check that protobuf message is not copied.
		// required for protobuf update, should be removed afterwards
		middlewares.Migration(copyproto.Analyzer),
		// check that protobuf messages are not compared using reflect.DeepEqual
		middlewares.Migration(deepequalproto.Analyzer),

		middlewares.Nolint(middlewares.Migration(bodyclose.Analyzer)),
		middlewares.Nolint(middlewares.Migration(rowserr.NewAnalyzer(
			"github.com/jmoiron/sqlx",
		))),

		// check that sql.Rows and sql.Stmt are closed
		middlewares.Nolint(middlewares.Migration(sqlclosecheck.NewAnalyzer())),

		middlewares.Nolint(middlewares.Migration(ytcheck.Analyzer)),

		structtagcase.Analyzer,
	}

	// staticcheck
	for _, v := range simple.Analyzers {
		analyzers = append(analyzers, middlewares.Nogen(middlewares.Nolint(v.Analyzer)))
	}
	for _, v := range staticcheck.Analyzers {
		analyzers = append(analyzers, middlewares.Nogen(middlewares.Nolint(v.Analyzer)))
	}
	for _, v := range stylecheck.Analyzers {
		analyzers = append(analyzers, middlewares.Migration(middlewares.Nogen(middlewares.Nolint(v.Analyzer))))
	}

	return middlewares.NamedAnalyzers(analyzers...)
}
