package uatraits

import (
	"encoding/xml"
	"testing"

	"github.com/stretchr/testify/assert"
)

// Вспомогательные функции для построения дерева.
func newPattern(type_ patternType, value, content string) *pattern {
	p := &pattern{
		XMLName: xml.Name{Local: "pattern"},
		Type:    type_,
		Value:   value,
		Content: content,
	}
	p.prebuildRegex()
	return p
}

func newMatch(patterns ...*pattern) *match {
	return &match{
		XMLName:  xml.Name{Local: "match"},
		Patterns: patterns[:],
	}
}

func newDefine(patterns ...*pattern) *definition {
	return &definition{
		XMLName:  xml.Name{Local: "define"},
		Patterns: patterns[:],
	}
}

func newBranch(name string, defines []*definition, matches []*match, children []*branch) *branch {
	return &branch{
		XMLName:  xml.Name{Local: "branch"},
		Name:     name,
		Defines:  defines,
		Matches:  matches,
		Children: children,
	}
}

func newRules(date, minVer string, branches ...*branch) *browserRules {
	return &browserRules{
		XMLName:    xml.Name{Local: "rules"},
		Date:       date,
		MinVersion: minVer,
		Branches:   branches[:],
	}
}

func TestParseBrowserRulesXMLBytes(t *testing.T) {

	t.Run("SeveralTopBranches", func(t *testing.T) {
		severalTopBranchesXML := []byte(`<?xml version="1.0?>
		<rules>
			<branch name="A"/>
			<branch name="B"/>
			<branch name="C"></branch>
		</rules>`)

		expected := newRules(
			"", "",
			newBranch("A", nil, nil, nil),
			newBranch("B", nil, nil, nil),
			newBranch("C", nil, nil, nil),
		)
		actual, err := parseBrowserRulesXMLBytes(severalTopBranchesXML)
		if assert.NoError(t, err) {
			assert.Equal(t, expected, actual)
		}
	})

	t.Run("ChildrenBranches", func(t *testing.T) {
		childrenBranchesXML := []byte(`<?xml version="1.0?>
		<rules>
			<branch name="A">
				<branch name="B">
					<branch name="C">
					</branch>
				</branch>
			</branch>
		</rules>`)

		expected := newRules(
			"", "",
			newBranch("A", nil, nil, []*branch{
				newBranch("B", nil, nil, []*branch{
					newBranch("C", nil, nil, nil),
				}),
			}),
		)
		actual, err := parseBrowserRulesXMLBytes(childrenBranchesXML)
		if assert.NoError(t, err) {
			assert.Equal(t, expected, actual)
		}
	})

	t.Run("Matches", func(t *testing.T) {
		matchesXML := []byte(`<?xml version="1.0"?>
		<rules>
			<branch name="A">
				<match type="any">
					<pattern type="string">alpha</pattern>
					<pattern type="regex">^beta</pattern>
				</match>
				<match>
					<pattern type="string">gamma</pattern>
				</match>
			</branch>
		</rules>
		`)

		expected := newRules(
			"", "",
			newBranch("A", nil, []*match{
				newMatch(
					newPattern(stringPatternType, "", "alpha"),
					newPattern(regexPatternType, "", "^beta"),
				),
				newMatch(
					newPattern(stringPatternType, "", "gamma"),
				),
			}, nil),
		)
		actual, err := parseBrowserRulesXMLBytes(matchesXML)
		if assert.NoError(t, err) {
			assert.Equal(t, expected, actual)
		}
	})
}

func Test_parseEntities(t *testing.T) {
	testCases := []struct {
		name      string
		directive xml.Directive
		want      map[string]string
	}{
		{
			name:      "empty_directive",
			directive: nil,
			want:      nil,
		},
		{
			name:      "invalid",
			directive: xml.Directive("ololo trololo"),
			want:      nil,
		},
		{
			name:      "empty_tag",
			directive: xml.Directive("<!ENTITY>"),
			want:      nil,
		},
		{
			name:      "valid_entity",
			directive: xml.Directive("<!ENTITY ver2d \"[0-9]+(?:[\\.][0-9]+)?\">"),
			want: map[string]string{
				"ver2d": "[0-9]+(?:[\\.][0-9]+)?",
			},
		},
		{
			name: "doctype",
			directive: xml.Directive(`<!DOCTYPE rules [
<!ENTITY ver1d "[0-9]+">
<!ENTITY ver2d "[0-9]+(?:[\.][0-9]+)?">
<!ENTITY ver3d "[0-9]+(?:[\.][0-9]+){0,2}">
<!ENTITY ver "[0-9][0-9\.]*">
<!ENTITY word "a-z0-9_">
<!ENTITY num "0-9">
<!ENTITY s " ">
<!ENTITY seps "[ \-/_]">
<!ENTITY model "[a-zA-Z0-9/\_\-\.\+'&quot; ][a-zA-Z0-9/\_\-\.\+'&quot;\(\)\\, ]*">
<!ENTITY b "[\(\) _]">
<!ENTITY notmodels "(?:Release\/&ver;|Build\/(?:(?:OP[RM]|PPR)?&ver;|[a-z0-9_\.]+)|en\-us|x96|device info|mobile|zh\-cn|ru_ru|Build|Network|tablet|arm|arm_64|Microsoft|nokia|wv|Lenovo| )">
<!ENTITY build "(?:(?: Build|/|;|\)| \().*)?$">
<!ENTITY androids "(?:Marshmallow|Nougat|Oreo|Pie|Lollipop|KitKat|Jelly Bean|Ice Cream Sandwich|Honeycomb|Gingerbread|Froyo|Eclair|Donut|Cupcake|Bender|Astro)">
]>`),
			want: map[string]string{
				"androids":  "(?:Marshmallow|Nougat|Oreo|Pie|Lollipop|KitKat|Jelly Bean|Ice Cream Sandwich|Honeycomb|Gingerbread|Froyo|Eclair|Donut|Cupcake|Bender|Astro)",
				"b":         "[\\(\\) _]",
				"build":     "(?:(?: Build|/|;|\\)| \\().*)?$",
				"model":     "[a-zA-Z0-9/\\_\\-\\.\\+'\" ][a-zA-Z0-9/\\_\\-\\.\\+'\"\\(\\)\\\\, ]*",
				"notmodels": "(?:Release\\/&ver;|Build\\/(?:(?:OP[RM]|PPR)?&ver;|[a-z0-9_\\.]+)|en\\-us|x96|device info|mobile|zh\\-cn|ru_ru|Build|Network|tablet|arm|arm_64|Microsoft|nokia|wv|Lenovo| )",
				"num":       "0-9",
				"s":         " ",
				"seps":      "[ \\-/_]",
				"ver":       "[0-9][0-9\\.]*",
				"ver1d":     "[0-9]+",
				"ver2d":     "[0-9]+(?:[\\.][0-9]+)?",
				"ver3d":     "[0-9]+(?:[\\.][0-9]+){0,2}",
				"word":      "a-z0-9_",
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			got := parseEntities(tc.directive)
			assert.Equal(t, tc.want, got)
		})
	}
}

func Benchmark_parseEntities(b *testing.B) {
	directive := xml.Directive(`<!DOCTYPE rules [
<!ENTITY ver1d "[0-9]+">
<!ENTITY ver2d "[0-9]+(?:[\.][0-9]+)?">
<!ENTITY ver3d "[0-9]+(?:[\.][0-9]+){0,2}">
<!ENTITY ver "[0-9][0-9\.]*">
<!ENTITY word "a-z0-9_">
<!ENTITY num "0-9">
<!ENTITY s " ">
<!ENTITY seps "[ \-/_]">
<!ENTITY model "[a-zA-Z0-9/\_\-\.\+'&quot; ][a-zA-Z0-9/\_\-\.\+'&quot;\(\)\\, ]*">
<!ENTITY b "[\(\) _]">
<!ENTITY notmodels "(?:Release\/&ver;|Build\/(?:(?:OP[RM]|PPR)?&ver;|[a-z0-9_\.]+)|en\-us|x96|device info|mobile|zh\-cn|ru_ru|Build|Network|tablet|arm|arm_64|Microsoft|nokia|wv|Lenovo| )">
<!ENTITY build "(?:(?: Build|/|;|\)| \().*)?$">
<!ENTITY androids "(?:Marshmallow|Nougat|Oreo|Pie|Lollipop|KitKat|Jelly Bean|Ice Cream Sandwich|Honeycomb|Gingerbread|Froyo|Eclair|Donut|Cupcake|Bender|Astro)">
]>`)

	b.ReportAllocs()
	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		parseEntities(directive)
	}
}
