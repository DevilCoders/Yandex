package uatraits

import (
	"encoding/xml"
)

type matchType string

const (
	anyMatchType = matchType("any")
)

type match struct {
	XMLName xml.Name `xml:"match"`
	// Typ     MatchType `xml:"type,attr,omitempty"` // <-- in fact it's unused at all
	Patterns []*pattern `xml:"pattern"`
}

func (m *match) prepare() {
	// pre-build regexps in patterns
	for _, p := range m.Patterns {
		p.prebuildRegex()
	}
}

func (m *match) isMatched(userAgent string) bool {
	for _, p := range m.Patterns {
		if p.isMatched(userAgent) {
			return true
		}
	}
	return false
}
