package godogutil

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/DATA-DOG/godog/gherkin"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// TestContext for entire godog feature(s)
type TestContext struct {
	strict   bool
	userInit func(tc *TestContext, s *godog.Suite)

	ctxPerScenario *ContextPerScenario
	subTests       *ScenarioAsSubtest
}

type TestContextOption func(*TestContext)

// WithContextPerScenario initializes separate context.Context with timeout for each scenario
func WithContextPerScenario() TestContextOption {
	return func(tc *TestContext) {
		tc.ctxPerScenario = &ContextPerScenario{}
	}
}

// WithScenariosAsSubtests adds go test imitation to output (so that ya make can parse it)
func WithScenariosAsSubtests() TestContextOption {
	return func(tc *TestContext) {
		tc.subTests = &ScenarioAsSubtest{Strict: tc.strict}
	}
}

func NewTestContext(strict bool, userInit func(tc *TestContext, s *godog.Suite), opts ...TestContextOption) *TestContext {
	tc := &TestContext{strict: strict, userInit: userInit}

	for _, opt := range opts {
		opt(tc)
	}

	return tc
}

func (tc *TestContext) godogInit(s *godog.Suite) {
	s.AfterStep(tc.AfterStep)

	if tc.ctxPerScenario != nil {
		tc.ctxPerScenario.Register(s)
	}

	if tc.subTests != nil {
		tc.subTests.Register(s)
	}

	tc.userInit(tc, s)
}

func (tc *TestContext) Context() context.Context {
	if tc.ctxPerScenario == nil {
		panic("nil context per scenario, probably forgot to create TestContext WithContextPerScenario")
	}

	return tc.ctxPerScenario.ctx
}

func (tc *TestContext) AfterStep(step *gherkin.Step, err error) {
	if err != nil {
		HoldOnError()
	}
}

type LastExecutedNames struct {
	FeatureName  string
	ScenarioName string
}

func (tc *TestContext) LastExecutedNames() LastExecutedNames {
	if tc.subTests == nil {
		panic("LastExecutedNames works only for WithScenariosAsSubtests")
	}
	return LastExecutedNames{
		FeatureName:  tc.subTests.getLastFeatureName(),
		ScenarioName: tc.subTests.getLastScenarioName(),
	}
}

const (
	defaultScenarioTimeout = time.Minute
)

type ContextPerScenario struct {
	ScenarioTimeout time.Duration

	ctx       context.Context
	ctxCancel context.CancelFunc
}

func (c *ContextPerScenario) Register(s *godog.Suite) {
	s.BeforeScenario(c.BeforeScenario)
	s.AfterScenario(c.AfterScenario)
}

func (c *ContextPerScenario) BeforeScenario(arg interface{}) {
	ctx := context.Background()

	t := defaultScenarioTimeout
	if c.ScenarioTimeout != 0 {
		t = c.ScenarioTimeout
	}

	c.ctx, c.ctxCancel = context.WithTimeout(ctx, t)
}

func (c *ContextPerScenario) AfterScenario(arg interface{}, err error) {
	c.ctxCancel()
	c.ctxCancel = func() {}
	c.ctx = nil
}

type scenarioResult struct {
	Name     string
	Duration time.Duration
	Err      error
}

func (sr scenarioResult) IsFailed(strict bool) bool {
	if sr.Err == nil {
		return false
	}

	if strict {
		return true
	}

	return !xerrors.Is(sr.Err, godog.ErrPending)
}

type ScenarioAsSubtest struct {
	Strict bool

	runningFeature    *gherkin.Feature
	featureStartTime  time.Time
	scenarioStartTime time.Time
	finishedScenarios []scenarioResult
}

func (c *ScenarioAsSubtest) Register(s *godog.Suite) {
	s.BeforeFeature(c.BeforeFeature)
	s.AfterFeature(c.AfterFeature)
	s.BeforeScenario(c.BeforeScenario)
	s.AfterScenario(c.AfterScenario)
}

func (c *ScenarioAsSubtest) BeforeFeature(f *gherkin.Feature) {
	// Set start time AFTER all the setup
	defer func() {
		c.featureStartTime = time.Now()
	}()

	c.runningFeature = f
	c.finishedScenarios = nil

	// Report we are strting a feature
	fmt.Printf("=== RUN %s\n", formatTestName(c.runningFeature.Name))
}

func (c *ScenarioAsSubtest) AfterFeature(_ *gherkin.Feature) {
	// Did we fail any scenarios?
	var failed bool
	for _, res := range c.finishedScenarios {
		if res.IsFailed(c.Strict) {
			failed = true
			break
		}
	}

	// Report feature result
	fmt.Printf(
		"--- %s: %s (%ss)\n", formatTestResult(failed),
		formatTestName(c.runningFeature.Name),
		formatTestDuration(time.Since(c.featureStartTime)),
	)

	// Report results of all scenarios
	for _, res := range c.finishedScenarios {
		fmt.Printf(
			"    --- %s: %s/%s (%ss)\n",
			formatTestResult(res.IsFailed(c.Strict)),
			formatTestName(c.runningFeature.Name),
			res.Name,
			formatTestDuration(res.Duration),
		)
	}
}

var outlineIndex = make(map[string]int)

func (c *ScenarioAsSubtest) BeforeScenario(arg interface{}) {
	// Set start time AFTER all the setup
	defer func() {
		c.scenarioStartTime = time.Now()
	}()

	// Report we are starting a scenario
	sc, ok := arg.(*gherkin.Scenario)
	if ok {
		fmt.Printf("=== RUN %s/%s\n", formatTestName(c.runningFeature.Name), sc.Name)
		return
	}

	so, ok := arg.(*gherkin.ScenarioOutline)
	if ok {
		// Gherkin does not have any spec regarding handling of escaped characters.
		// So behave does it by only handling \| as special case and leaving other bytes as is.
		// And godog replaces a pair of \n symbols with actual one-byte newline symbol.
		// Replace it back for example table values
		for _, tb := range so.Examples[0].TableBody {
			for _, c := range tb.Cells {
				c.Value = strings.Replace(c.Value, "\n", "\\n", -1)
			}
		}
		// We do not save outline index here, we do than when we increment the counter in AfterScenario
		i := outlineIndex[so.Name]
		name := fmt.Sprintf("%s_%d", so.Name, i)
		fmt.Printf("=== RUN %s/%s\n", formatTestName(c.runningFeature.Name), name)
		return
	}

	panic(fmt.Sprintf("unknown arg: %+v", arg))
}

func (c *ScenarioAsSubtest) AfterScenario(arg interface{}, err error) {
	duration := time.Since(c.scenarioStartTime)

	// Save scenario result
	sc, ok := arg.(*gherkin.Scenario)
	if ok {
		c.finishedScenarios = append(c.finishedScenarios, scenarioResult{Name: sc.Name, Duration: duration, Err: err})
		return
	}

	so, ok := arg.(*gherkin.ScenarioOutline)
	if ok {
		name := fmt.Sprintf("%s_%d", so.Name, outlineIndex[so.Name])
		outlineIndex[so.Name]++
		c.finishedScenarios = append(c.finishedScenarios, scenarioResult{Name: name, Duration: duration, Err: err})
		return
	}

	panic(fmt.Sprintf("unknown arg: %+v", arg))
}

func (c *ScenarioAsSubtest) getLastScenarioName() string {
	if len(c.finishedScenarios) == 0 {
		panic("attempt to getLastScenarioName when no Scenario is finished")
	}
	return c.finishedScenarios[len(c.finishedScenarios)-1].Name
}

func (c *ScenarioAsSubtest) getLastFeatureName() string {
	return c.runningFeature.Name
}

func formatTestResult(fail bool) string {
	if fail {
		return "FAIL"
	}

	return "PASS"
}

func formatTestName(n string) string {
	return "Test_" + n
}
