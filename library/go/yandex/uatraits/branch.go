package uatraits

import (
	"encoding/xml"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type branchType string

const (
	defaultBranchType = branchType("default")
	commonBranchType  = branchType("common")
)

type branch struct {
	XMLName xml.Name   `xml:"branch"`
	Name    string     `xml:"name,attr"`
	Type    branchType `xml:"type,attr,omitempty"`

	Matches  []*match // <-- in fact there is maximum one match in a branch
	Defines  []*definition
	Children []*branch
}

func (b *branch) isDefault() bool {
	return b.Type == defaultBranchType
}

func (b *branch) isCommon() bool {
	return b.Type == commonBranchType
}

func (b *branch) isMatched(userAgent string) bool {
	// check all matches
	for _, m := range b.Matches {
		if m.isMatched(userAgent) {
			return true
		}
	}
	return false
}

func (b *branch) trigger(userAgent string, traits Traits) {
	// Trigger all definitions in branch
	for _, define := range b.Defines {
		_ = define.trigger(userAgent, traits)
	}

	isWorked := false
	var defaultBranch *branch

	// Trigger all children branches.
	for _, child := range b.Children {
		if child.isDefault() {
			// Default branch which will be triggered when no other was.
			defaultBranch = child
		} else if child.isCommon() {
			// Branches with "common" attribute are always triggered.
			child.trigger(userAgent, traits)
		} else if !isWorked && child.isMatched(userAgent) {
			isWorked = true
			child.trigger(userAgent, traits)
		}
	}

	if !isWorked && defaultBranch != nil {
		defaultBranch.trigger(userAgent, traits)
	}
}

func (b *branch) UnmarshalXML(d *xml.Decoder, start xml.StartElement) error {
	b.XMLName = start.Name
	for _, attr := range start.Attr {
		switch attr.Name.Local {
		case "name":
			b.Name = attr.Value
		case "type":
			b.Type = branchType(attr.Value)
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
			case "match":
				m := &match{}
				if err := d.DecodeElement(m, &tt); err != nil {
					return err
				}
				m.prepare()
				b.Matches = append(b.Matches, m)
			case "define":
				define := &definition{}
				if err := d.DecodeElement(define, &tt); err != nil {
					return err
				}
				define.prepare()
				b.Defines = append(b.Defines, define)
			case "branch":
				childBranch := &branch{}
				if err := d.DecodeElement(childBranch, &tt); err != nil {
					return err
				}
				b.Children = append(b.Children, childBranch)
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
