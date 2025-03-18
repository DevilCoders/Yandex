package uatraits

import (
	"bytes"
	"encoding/xml"
	"html"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

var (
	entityTagStart = []byte(`<!ENTITY `)
	entityTagEnd   = []byte(`">`)
)

func parseEntities(directive xml.Directive) map[string]string {
	// this bytes will separate useful entity payload after slicing
	contentSep := []byte(` "`)

	var si, ei int

	entities := make(map[string]string)
	for {
		// start index of tag
		si = bytes.Index(directive, entityTagStart)
		if si == -1 {
			break
		}
		// end index of tag
		ei = bytes.Index(directive[si:], entityTagEnd)
		if ei == -1 {
			break
		}

		// get slice with tag content
		content := directive[si+len(entityTagStart) : si+ei]
		// reduce haystack
		directive = directive[ei:]

		// separator index
		si = bytes.Index(content, contentSep)
		if si == -1 {
			continue
		}

		key := content[:si]
		value := content[si+len(contentSep):]

		if err := valid.Alphanumeric(string(key)); err != nil {
			continue
		}

		entities[string(key)] = html.UnescapeString(string(value))
	}

	if len(entities) == 0 {
		return nil
	}
	return entities
}

func parseBrowserRulesXMLBytes(xmlBytes []byte) (*browserRules, error) {
	rules := &browserRules{}

	decoder := xml.NewDecoder(bytes.NewReader(xmlBytes))

	// copy default HTML entities
	decoder.Entity = make(map[string]string)
	for k, v := range xml.HTMLEntity {
		decoder.Entity[k] = v
	}

	// Read ENTITY directives
	var rulesStartElement xml.StartElement
	for {
		currentToken, err := decoder.Token()
		if err != nil {
			return nil, xerrors.Errorf("cannot parse XML bytes: %w", err)
		}
		currentToken = xml.CopyToken(currentToken)
		if startElementToken, ok := currentToken.(xml.StartElement); ok {
			rulesStartElement = startElementToken
			break
		}
		if directive, ok := currentToken.(xml.Directive); ok {
			for k, v := range parseEntities(directive) {
				decoder.Entity[k] = v
			}
		}
	}

	if err := decoder.DecodeElement(rules, &rulesStartElement); err != nil {
		return nil, xerrors.Errorf("cannot parse XML bytes: %w", err)
	}

	return rules, nil
}
