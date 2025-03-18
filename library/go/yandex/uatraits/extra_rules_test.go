package uatraits

import (
	"encoding/xml"
	"testing"

	"github.com/stretchr/testify/assert"
)

func createCondition(type_ comparisonType, field, value string) *compareCondition {
	return &compareCondition{
		typ:   type_,
		Field: field,
		Value: value,
	}
}

func createGroup(type_ string, conditions ...Condition) *groupCondition {
	return &groupCondition{
		XMLName: xml.Name{Local: "group"},
		Type:    type_,
		Items:   conditions[:],
	}
}

func createRule(name, value string, root *groupCondition) rule {
	return rule{
		XMLName: xml.Name{Local: "rule"},
		Name:    name,
		Value:   value,
		Root:    root,
	}
}

func createExtraRules(minVer string, rules ...rule) extraRules {
	return extraRules{
		XMLName:        xml.Name{Local: "rules"},
		MinimalVersion: minVer,
		Rules:          rules[:],
	}
}

func TestParseExtraRulesBytes(t *testing.T) {
	rulesXML := []byte(`
<?xml version="1.0" encoding="UTF-8"?>
<rules minver="1.2.3">
	<rule name="featureAlpha" value="true">
		<group type="and">
		</group>
	</rule>
	<rule name="featureBeta" value="beta">
		<group type="or">
			<group type="and">
				<eq field="BrowserBase">Safari</eq>
                <gte field="BrowserBaseVersion">11</gte>
			</group>
			<group type="and">
				<eq field="BrowserBase">YaBro</eq>
                <lt field="BrowserBaseVersion">42.0</lt>
			</group>
			<group type="and">
				<neq field="BrowserBase">IE</neq>
                <lte field="BrowserBaseVersion">99</lte>
			</group>
		</group>
	</rule>
</rules>
`)

	expectedRules := createExtraRules("1.2.3",
		createRule("featureAlpha", "true",
			createGroup("and"),
		),
		createRule("featureBeta", "beta",
			createGroup("or",
				createGroup("and",
					createCondition(equal, "BrowserBase", "Safari"),
					createCondition(greaterOrEqual, "BrowserBaseVersion", "11"),
				),
				createGroup("and",
					createCondition(equal, "BrowserBase", "YaBro"),
					createCondition(less, "BrowserBaseVersion", "42.0"),
				),
				createGroup("and",
					createCondition(notEqual, "BrowserBase", "IE"),
					createCondition(lessOrEqual, "BrowserBaseVersion", "99"),
				),
			),
		),
	)

	actualRules, err := parseExtraRulesXMLBytes(rulesXML)
	if !assert.NoError(t, err) {
		assert.FailNow(t, "cannot parse extra rules", err)
	}

	assert.Equal(t, &expectedRules, actualRules)
}

func TestTriggerExtraRules(t *testing.T) {
	rules := createExtraRules("",
		createRule("alpha", "ALPHA",
			createGroup("and",
				createCondition(equal, "BrowserBase", "Chromium"),
			),
		),
		createRule("beta", "BETA",
			createGroup("or",
				createGroup("and",
					createCondition(equal, "BrowserName", "SuperBrowser"),
					createCondition(greaterOrEqual, "BrowserVersion", "11.0"),
				),
				createGroup("and",
					createCondition(equal, "BrowserName", "AnotherBrowser"),
					createCondition(less, "BrowserVersion", "12.0"),
				),
			),
		),
		createRule("not_equal", "true",
			createGroup("and",
				createCondition(notEqual, "BrowserBaseVersion", "42"),
			),
		),
		createRule("equal", "true",
			createGroup("and",
				createCondition(equal, "BrowserBaseVersion", "42"),
			),
		),
		createRule("less_than", "true",
			createGroup("and",
				createCondition(less, "BrowserBaseVersion", "42"),
			),
		),
		createRule("greater_than_equal", "true",
			createGroup("and",
				createCondition(greaterOrEqual, "BrowserBaseVersion", "42"),
			),
		),
		createRule("less_than_equal", "true",
			createGroup("and",
				createCondition(lessOrEqual, "BrowserBaseVersion", "42"),
			),
		),
	)

	t.Run("SimpleRule", func(t *testing.T) {
		traits := newTraits()
		traits.Set("BrowserBase", "Chromium")

		rules.trigger(traits)

		assert.Equal(t, "ALPHA", traits.Get("alpha"))
	})
	t.Run("ComplexRule", func(t *testing.T) {
		traits := newTraits()
		traits.Set("BrowserName", "SuperBrowser")
		traits.Set("BrowserVersion", "11.5")

		rules.trigger(traits)

		assert.Equal(t, "BETA", traits.Get("beta"))
	})
	t.Run("DifferentComparisonOperators", func(t *testing.T) {
		for _, subtest := range []struct {
			ExpectedFlags      []string
			BrowserBaseVersion string
		}{
			{[]string{"not_equal", "less_than", "less_than_equal"}, "41"},
			{[]string{"equal", "greater_than_equal", "less_than_equal"}, "42"},
			{[]string{"not_equal", "greater_than_equal"}, "43"},
		} {
			expectedTraits := newTraits()
			expectedTraits.Set("BrowserBaseVersion", subtest.BrowserBaseVersion)
			for _, flag := range subtest.ExpectedFlags {
				expectedTraits.Set(flag, "true")
			}

			traits := newTraits()
			traits.Set("BrowserBaseVersion", subtest.BrowserBaseVersion)
			rules.trigger(traits)

			assert.Equal(t, expectedTraits, traits, "BrowserBaseVersion = %s", expectedTraits.BrowserBaseVersion)
		}
	})
}
