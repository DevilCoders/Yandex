package uatraits

import (
	"encoding/xml"
)

type browserRules struct {
	XMLName    xml.Name  `xml:"rules"`
	Branches   []*branch `xml:"branch"` // <-- at least one branch should exist
	Date       string    `xml:"date,attr"`
	MinVersion string    `xml:"minver,attr"`

	rootBranch *branch
}

func (rules *browserRules) trigger(userAgent string, traits Traits) {
	rules.rootBranch = &branch{
		Children: rules.Branches,
	}
	rules.rootBranch.trigger(userAgent, traits)
}
