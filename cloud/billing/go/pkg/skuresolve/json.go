package skuresolve

import (
	"fmt"
	"strings"

	"github.com/valyala/fastjson"
)

// FastJSONGetter is MetricValueGetter implementation from arbitrary json
type FastJSONGetter struct {
	root *fastjson.Value
}

// NewFastJSONGetter by parsed fastjson
func NewFastJSONGetter(root *fastjson.Value) (*FastJSONGetter, error) {
	if root.Type() != fastjson.TypeObject {
		return nil, fmt.Errorf("incorrect root element type %s", root.Type().String())
	}
	return &FastJSONGetter{root: root}, nil
}

// ParseToFastJSONGetter parses json and makes getter
func ParseToFastJSONGetter(value string) (*FastJSONGetter, error) {
	root, err := fastjson.Parse(value)
	if err != nil {
		return nil, err
	}
	return NewFastJSONGetter(root)
}

func (g *FastJSONGetter) MatchByPath(path string, matcher MetricValueMatcher) (bool, error) {
	fjPath := g.splitPath(path)
	return matcher.MatchJSON(g.root.Get(fjPath...)), nil
}

func (g *FastJSONGetter) GetFullValue() *fastjson.Value {
	return g.root
}

func (g *FastJSONGetter) splitPath(p string) []string {
	p = strings.ReplaceAll(p, "]", "")
	return strings.FieldsFunc(p, func(r rune) bool { return r == '.' || r == '[' })
}
