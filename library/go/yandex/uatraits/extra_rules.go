package uatraits

import (
	"encoding/xml"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Compares values as some software version values (e.g. "1.20.3")
// and returns -1, 0, 1 for less, equal or great result accordingly
// or notnil error if parsing issue occured.
func compareAsVersions(valueOne, valueTwo string) (int, error) {
	v1, err := parseVersion(valueOne)
	if err != nil {
		return 0, err
	}
	v2, err := parseVersion(valueTwo)
	if err != nil {
		return 0, err
	}
	return v1.CompareTo(v2), nil
}

// Firstly tries to compare the values as versions and if some error is presented,
// compares as usual strings.
// Returns -1, 0, 1 for less, equal or great result accordingly
func compare(valueOne, valueTwo string) int {
	if result, err := compareAsVersions(valueOne, valueTwo); err == nil {
		return result
	}
	return strings.Compare(valueOne, valueTwo)
}

type Condition interface {
	Check(Traits) bool
}

type comparisonType int

const (
	equal = comparisonType(iota)
	notEqual
	less
	greater
	lessOrEqual
	greaterOrEqual
	undefined
)

func parseComparisonType(value string) comparisonType {
	switch value {
	case "eq":
		return equal
	case "neq":
		return notEqual
	case "lt":
		return less
	case "gt":
		return greater
	case "lte":
		return lessOrEqual
	case "gte":
		return greaterOrEqual
	default:
		return undefined
	}
}

type compareCondition struct {
	Field string `xml:"field,attr"`
	Value string `xml:",chardata"`
	typ   comparisonType
}

func (condition *compareCondition) Check(traits Traits) bool {
	result := compare(traits.Get(condition.Field), condition.Value)
	switch condition.typ {
	case equal:
		return result == 0
	case notEqual:
		return result != 0
	case less:
		return result == -1
	case greater:
		return result == 1
	case lessOrEqual:
		return result != 1
	case greaterOrEqual:
		return result != -1
	default:
		return false
	}
}

type groupCondition struct {
	XMLName xml.Name    `xml:"group"`
	Type    string      `xml:"type,attr"`
	Items   []Condition `xml:",innerxml"`
}

func (condition *groupCondition) UnmarshalXML(d *xml.Decoder, start xml.StartElement) error {
	condition.XMLName = start.Name

	for _, attr := range start.Attr {
		if attr.Name.Local == "type" {
			condition.Type = attr.Value
		}
	}

	for {
		t, err := d.Token()
		if err != nil {
			return err
		}
		switch tt := t.(type) {
		case xml.StartElement:
			switch tt.Name.Local {
			case "group":
				group := &groupCondition{}
				if err := d.DecodeElement(group, &tt); err != nil {
					return err
				}
				condition.Items = append(condition.Items, group)
			case "eq", "neq", "lt", "gt", "lte", "gte":
				cond := &compareCondition{
					typ: parseComparisonType(tt.Name.Local),
				}
				if err := d.DecodeElement(cond, &tt); err != nil {
					return err
				}
				condition.Items = append(condition.Items, cond)
			default:
				return xerrors.Errorf("unexpected branch child: %s", tt.Name.Local)
			}
		case xml.EndElement:
			if tt == start.End() {
				return nil
			}
		}
	}
}

func (condition *groupCondition) checkAny(traits Traits) bool {
	for _, condition := range condition.Items {
		if condition.Check(traits) {
			return true
		}
	}
	return false
}

func (condition *groupCondition) checkAll(traits Traits) bool {
	for _, condition := range condition.Items {
		if !condition.Check(traits) {
			return false
		}
	}
	return true
}

func (condition *groupCondition) Check(traits Traits) bool {
	switch condition.Type {
	case "and":
		return condition.checkAll(traits)
	case "or":
		return condition.checkAny(traits)
	default:
		return false
	}
}

type rule struct {
	XMLName xml.Name `xml:"rule"`
	Name    string   `xml:"name,attr"`
	Value   string   `xml:"value,attr"`

	Root *groupCondition
}

func (r *rule) trigger(traits Traits) bool {
	if r.Root.Check(traits) {
		traits.Set(r.Name, r.Value)
		return true
	}
	return false
}

type extraRules struct {
	XMLName        xml.Name `xml:"rules"`
	MinimalVersion string   `xml:"minver,attr"`
	Rules          []rule   `xml:"rule"`
}

func (rules *extraRules) trigger(traits Traits) bool {
	for _, rule := range rules.Rules {
		_ = rule.trigger(traits)
	}
	return true
}

func parseExtraRulesXMLBytes(bytes []byte) (*extraRules, error) {
	rules := &extraRules{}
	if err := xml.Unmarshal(bytes, rules); err != nil {
		return nil, xerrors.Errorf("cannot parse extra rules: %w", err)
	}

	return rules, nil
}
