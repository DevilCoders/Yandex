package jmesengine

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/stretchr/testify/suite"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/jmesparse"
	"a.yandex-team.ru/library/go/test/yatest"
)

type TestSuite struct {
	Given     json.RawMessage
	TestCases []TestCase `json:"cases"`
	Comment   string
}

type TestCase struct {
	Comment    string
	Expression string
	Result     json.RawMessage
	Error      string
}

var whiteListed = []string{
	"/basic.json",
	"/current.json",
	"/escape.json",
	"/filters.json",
	"/functions.json",
	"/identifiers.json",
	"/indices.json",
	"/literal.json",
	"/multiselect.json",
	"/ormatch.json",
	"/pipe.json",
	"/slice.json",
	"/syntax.json",
	"/unicode.json",
	"/wildcard.json",
	"/boolean.json",

	"/custom.json",
}

type complianceTestSuite struct {
	suite.Suite
	complianceFiles []string
}

func TestCompliance(t *testing.T) {
	suite.Run(t, new(complianceTestSuite))
}

var compliancePath = yatest.SourcePath("cloud/billing/go/pkg/jmesengine/compliance/") + "/"

func (suite *complianceTestSuite) SetupSuite() {
	err := filepath.Walk(compliancePath, func(path string, _ os.FileInfo, _ error) error {
		// if strings.HasSuffix(path, ".json") {
		if suite.allowed(path) {
			suite.complianceFiles = append(suite.complianceFiles, path)
		}
		return nil
	})
	suite.Require().NoError(err)
	suite.Require().NotEmpty(suite.complianceFiles)
}

func (suite *complianceTestSuite) TestCompliance() {
	for _, path := range suite.complianceFiles {
		name := filepath.Base(path)
		suite.Run(name, func() {
			var testSuites []TestSuite
			data, err := ioutil.ReadFile(path)
			suite.Require().NoError(err)
			err = json.Unmarshal(data, &testSuites)
			suite.Require().NoError(err)
			for i, testsuite := range testSuites {
				suite.Run(fmt.Sprintf("suite-%d", i), func() {
					suite.runTestSuite(testsuite)
				})
			}
		})
	}
}

func (suite *complianceTestSuite) allowed(path string) bool {
	for _, el := range whiteListed {
		if strings.HasSuffix(path, el) {
			return true
		}
	}
	return false
}

func (suite *complianceTestSuite) runTestSuite(testsuite TestSuite) {
	for _, testcase := range testsuite.TestCases {
		if testcase.Error != "" {
			// This is a test case that verifies we error out properly.
			suite.Run(fmt.Sprintf("syntax-%s", testcase.Expression), func() {
				suite.runSyntaxTestCase(testsuite.Given, testcase)
			})
		} else {
			suite.Run(fmt.Sprintf("case-%s", testcase.Expression), func() {
				suite.runTestCase(testsuite.Given, testcase)
			})
		}
	}
}

func (suite *complianceTestSuite) runSyntaxTestCase(given []byte, testcase TestCase) {
	// Anything with an .Error means that we expect that JMESPath should return
	// an error when we try to evaluate the expression.
	ast, _ := jmesparse.NewParser().Parse(testcase.Expression)
	json := fastjson.MustParseBytes(given)
	intr := NewInterpreter()

	_, err := intr.Execute(ast, Value(json))
	suite.Require().Error(err, "Expression: %s", testcase.Expression)
}

func (suite *complianceTestSuite) runTestCase(given []byte, testcase TestCase) {
	parser := jmesparse.NewParser()
	_, err := parser.Parse(testcase.Expression)
	suite.Require().NoError(err, "Could not parse expression: %s", testcase.Expression)

	ast, _ := jmesparse.NewParser().Parse(testcase.Expression)
	json := fastjson.MustParseBytes(given)
	intr := NewInterpreter()

	actual, err := intr.Execute(ast, Value(json))
	suite.Require().NoError(err, "Expression: %s", testcase.Expression)

	wantStr := string(testcase.Result)
	actualStr := string(actual.Value().MarshalTo(nil))

	suite.JSONEq(wantStr, actualStr, "Expression: %s", testcase.Expression)
}
