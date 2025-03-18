package uatraits

import "encoding/xml"

type definition struct {
	XMLName  xml.Name   `xml:"define"`
	Patterns []*pattern `xml:"pattern"`
	Name     string     `xml:"name,attr"`
	Value    string     `xml:"value,attr,omitempty"`
	Content  string     `xml:",chardata"`
}

func (d *definition) prepare() {
	for i := range d.Patterns {
		d.Patterns[i].setParentName(d.Name)
		d.Patterns[i].prebuildRegex()
	}
}

func (d *definition) trigger(userAgent string, traits Traits) bool {
	// no pattern means that it's a static definition
	if len(d.Patterns) == 0 {
		/*
			Static definition can be described in two ways:
				<define name="OSFamily" value="macOS"/>
				or
				<define name="OSFamily">macOS</define>.
			Both of these set "OSFamily" trait to be "macOS"
		*/
		if d.Value != "" {
			traits.Set(d.Name, d.Value)
		} else {
			traits.Set(d.Name, d.Content)
		}

		return true
	}
	for _, p := range d.Patterns {
		if p.trigger(userAgent, traits) {
			return true
		}
	}
	return false
}
