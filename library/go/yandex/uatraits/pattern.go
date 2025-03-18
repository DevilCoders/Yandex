package uatraits

import (
	"encoding/xml"
	"fmt"
	"regexp"
	"strings"
)

type patternType string

const (
	stringPatternType     = patternType("string")
	regexPatternType      = patternType("regex")
	caseInsensitivePrefix = "(?i)"
)

type pattern struct {
	XMLName xml.Name    `xml:"pattern"`
	Type    patternType `xml:"type,attr"`
	Value   string      `xml:"value,attr,omitempty"`
	Content string      `xml:",chardata"`
	Name    string

	// Applicable only for regex type.
	regex *regexp.Regexp
}

func (p *pattern) setParentName(parentName string) {
	p.Name = parentName
}

func (p *pattern) prebuildRegex() {
	if p.Type == regexPatternType && p.regex == nil {
		p.regex = regexp.MustCompile(caseInsensitivePrefix + p.Content)
	}
}

func (p *pattern) triggerString(userAgent string, traits Traits) bool {
	if сontainsIgnoreCase(userAgent, p.Content) {
		traits.Set(p.Name, p.Value)
		return true
	}
	return false
}

func (p *pattern) triggerRegex(userAgent string, traits Traits) bool {
	if regexMatch := p.regex.FindStringSubmatch(userAgent); regexMatch != nil {
		value := p.Value
		for i, submatch := range regexMatch[1:] {
			value = strings.ReplaceAll(value, fmt.Sprintf("$%d", i+1), submatch)
		}
		traits.Set(p.Name, value)
		return true
	}
	return false
}

func (p *pattern) trigger(userAgent string, traits Traits) bool {
	switch p.Type {
	case stringPatternType:
		return p.triggerString(userAgent, traits)
	case regexPatternType:
		return p.triggerRegex(userAgent, traits)
	default:
		return false
	}
}

func (p *pattern) isMatched(userAgent string) bool {
	switch p.Type {
	case stringPatternType:
		return сontainsIgnoreCase(userAgent, p.Content)
	case regexPatternType:
		return p.regex.MatchString(userAgent)
	default:
		return false
	}
}
