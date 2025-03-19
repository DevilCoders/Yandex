package actions

import (
	"fmt"
	"strings"
	"sync"

	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/pkg/skuresolve"
)

type metricValueGetter struct {
	metric    *entities.SourceMetric
	tags      *fastjson.Value
	fullValue *fastjson.Value
	parser    *fastjson.Parser
	arena     *fastjson.Arena
	parseOnce sync.Once
}

func (g *metricValueGetter) MatchByPath(path string, matcher skuresolve.MetricValueMatcher) (bool, error) {
	switch path { // known paths in metric
	case "usage.quantity":
		return matcher.MatchValue(g.metric.Usage.Quantity), nil
	case "usage.start":
		return matcher.MatchValue(g.metric.Usage.Start.Unix()), nil
	case "usage.finish":
		return matcher.MatchValue(g.metric.Usage.Finish.Unix()), nil
	case "usage.unit":
		return matcher.MatchValue(g.metric.Usage.Unit), nil
	case "usage.type":
		return matcher.MatchValue(g.metric.Usage.RawType), nil
	case "schema":
		return matcher.MatchValue(g.metric.Schema), nil
	case "version":
		return matcher.MatchValue(g.metric.Version), nil
	}
	if strings.HasPrefix(path, "tags.") {
		return g.matchTag(strings.TrimPrefix(path, "tags."), matcher), nil
	}
	return false, ErrSkuFormula.Wrap(fmt.Errorf("path does not supported %s", path))
}

func (g *metricValueGetter) GetFullValue() *fastjson.Value {
	g.parse()
	return g.fullValue
}

func (g *metricValueGetter) matchTag(path string, matcher skuresolve.MetricValueMatcher) bool {
	g.parse()

	if g.tags == nil {
		return false
	}

	fjPath := parseFastJSONPath(path)
	return matcher.MatchJSON(g.tags.Get(fjPath...))
}

func (g *metricValueGetter) parse() {
	g.parseOnce.Do(func() {
		g.parser = parsers.Get()
		g.arena = arenas.Get()

		if len(g.metric.Tags) > 0 {
			var err error
			g.tags, err = g.parser.Parse(string(g.metric.Tags))
			if err != nil {
				// tags is part of bigger json message and should be parsed without errors
				// this panic is mainly for tests debug
				panic(fmt.Errorf("tags parse error: %w", err))
			}
		}
		g.fullValue = g.arena.NewObject()

		usage := g.arena.NewObject()
		quantity := g.arena.NewNumberString(g.metric.Usage.Quantity.String())
		start := g.arena.NewNumberInt(int(g.metric.Usage.Start.Unix()))
		finish := g.arena.NewNumberInt(int(g.metric.Usage.Finish.Unix()))
		unit := g.arena.NewString(g.metric.Usage.Unit)
		uType := g.arena.NewString(g.metric.Usage.RawType)
		schema := g.arena.NewString(g.metric.Schema)
		version := g.arena.NewString(g.metric.Version)

		usage.Set("quantity", quantity)
		usage.Set("start", start)
		usage.Set("finish", finish)
		usage.Set("unit", unit)
		usage.Set("type", uType)
		g.fullValue.Set("schema", schema)
		g.fullValue.Set("version", version)
		g.fullValue.Set("usage", usage)
		g.fullValue.Set("tags", g.tags)
	})
}

func (g *metricValueGetter) reset() {
	if g.parser == nil {
		return
	}

	g.tags = nil
	g.fullValue = nil
	parsers.Put(g.parser)
	arenas.Put(g.arena)
	g.parser = nil
	g.arena = nil
}

func parseFastJSONPath(p string) []string {
	p = strings.ReplaceAll(p, "]", "")
	return strings.FieldsFunc(p, func(r rune) bool { return r == '.' || r == '[' })
}
