package tags

import (
	"context"
	"time"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
)

type StringTagName ext.StringTagName

func (tag StringTagName) Tag(value string) opentracing.Tag {
	return opentracing.Tag{Key: string(tag), Value: value}
}

func (tag StringTagName) Set(span opentracing.Span, value string) {
	ext.StringTagName(tag).Set(span, value)
}

func (tag StringTagName) Setter(value string) Setter {
	return func(span opentracing.Span) {
		tag.Set(span, value)
	}
}

func (tag StringTagName) SetContext(ctx context.Context, value string) {
	SetContext(ctx, tag.Setter(value))
}

type StringsTagName string

func (tag StringsTagName) Tag(value []string) opentracing.Tag {
	return opentracing.Tag{Key: string(tag), Value: value}
}

func (tag StringsTagName) Set(span opentracing.Span, value []string) {
	span.SetTag(string(tag), value)
}

func (tag StringsTagName) Setter(value []string) Setter {
	return func(span opentracing.Span) {
		tag.Set(span, value)
	}
}

func (tag StringsTagName) SetContext(ctx context.Context, value []string) {
	SetContext(ctx, tag.Setter(value))
}

type BoolTagName ext.BoolTagName

func (tag BoolTagName) Tag(value bool) opentracing.Tag {
	return opentracing.Tag{Key: string(tag), Value: value}
}

func (tag BoolTagName) Set(span opentracing.Span, value bool) {
	ext.BoolTagName(tag).Set(span, value)
}

func (tag BoolTagName) Setter(value bool) Setter {
	return func(span opentracing.Span) {
		tag.Set(span, value)
	}
}

func (tag BoolTagName) SetContext(ctx context.Context, value bool) {
	SetContext(ctx, tag.Setter(value))
}

type IntTagName string

func (tag IntTagName) Tag(value int) opentracing.Tag {
	return opentracing.Tag{Key: string(tag), Value: value}
}

func (tag IntTagName) Set(span opentracing.Span, value int) {
	span.SetTag(string(tag), value)
}

func (tag IntTagName) Setter(value int) Setter {
	return func(span opentracing.Span) {
		tag.Set(span, value)
	}
}

func (tag IntTagName) SetContext(ctx context.Context, value int) {
	SetContext(ctx, tag.Setter(value))
}

type Int64TagName string

func (tag Int64TagName) Tag(value int64) opentracing.Tag {
	return opentracing.Tag{Key: string(tag), Value: value}
}

func (tag Int64TagName) Set(span opentracing.Span, value int64) {
	span.SetTag(string(tag), value)
}

func (tag Int64TagName) Setter(value int64) Setter {
	return func(span opentracing.Span) {
		tag.Set(span, value)
	}
}

func (tag Int64TagName) SetContext(ctx context.Context, value int64) {
	SetContext(ctx, tag.Setter(value))
}

type DurationTagName string

func (tag DurationTagName) Tag(value time.Duration) opentracing.Tag {
	return opentracing.Tag{Key: string(tag), Value: value}
}

func (tag DurationTagName) Set(span opentracing.Span, value time.Duration) {
	span.SetTag(string(tag), value)
}

func (tag DurationTagName) Setter(value time.Duration) Setter {
	return func(span opentracing.Span) {
		tag.Set(span, value)
	}
}

func (tag DurationTagName) SetContext(ctx context.Context, value time.Duration) {
	SetContext(ctx, tag.Setter(value))
}
