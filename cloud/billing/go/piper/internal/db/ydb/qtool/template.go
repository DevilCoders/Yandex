package qtool

import (
	"fmt"
	"strings"
)

type Template []templatePart

type QueryParams struct {
	RootPath string
	DB       string
}

func (t Template) WithParams(param QueryParams) string {
	b := strings.Builder{}
	for _, p := range t {
		_, _ = b.WriteString(p.render(param))
	}
	return b.String()
}

func (t Template) simplify() Template {
	result := make(Template, 0)
	b := strings.Builder{}
	for _, p := range t {
		if static, ok := p.(staticTemplatePart); ok {
			_, _ = b.WriteString(static.renderStatic())
			continue
		}
		if b.Len() > 0 {
			result = append(result, stringTemplate(b.String()))
			b.Reset()
		}
		result = append(result, p)
	}
	if b.Len() > 0 {
		result = append(result, stringTemplate(b.String()))
		b.Reset()
	}
	return result
}

func queryStringToTemplate(s QueryString) []templatePart {
	if v, ok := s.(string); ok {
		return []templatePart{stringTemplate(v)}
	}
	return queryLineToTemplate(s)
}

func queryLineToTemplate(s QueryString) []templatePart {
	switch v := s.(type) {
	case string:
		return []templatePart{templateLine(v)}
	case templatePart:
		return []templatePart{v}
	case Template:
		return v
	}
	panic(fmt.Sprintf("unknown query string type %T", s))
}

type templatePart interface {
	render(QueryParams) string
}

type staticTemplatePart interface {
	renderStatic() string
}

type tableName string

func (s tableName) render(param QueryParams) string {
	b := strings.Builder{}
	_, _ = b.WriteString(" `")
	if param.RootPath != "" {
		_, _ = b.WriteString(param.RootPath)
	}
	_, _ = b.WriteString(string(s))
	_, _ = b.WriteString("` ")

	return b.String()
}

type stringTemplate string

func templateLine(l string) stringTemplate {
	return stringTemplate(l + "\n")
}

func (s stringTemplate) render(QueryParams) string { return s.renderStatic() }
func (s stringTemplate) renderStatic() string      { return string(s) }
