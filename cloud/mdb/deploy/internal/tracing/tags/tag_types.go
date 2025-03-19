package tags

import (
	"context"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
)

type shipmentIDTagName string

func (tag shipmentIDTagName) Tag(value models.ShipmentID) opentracing.Tag {
	return opentracing.Tag{Key: string(tag), Value: value}
}

func (tag shipmentIDTagName) Set(span opentracing.Span, value models.ShipmentID) {
	span.SetTag(string(tag), value)
}

func (tag shipmentIDTagName) Setter(value models.ShipmentID) tags.Setter {
	return func(span opentracing.Span) {
		tag.Set(span, value)
	}
}

func (tag shipmentIDTagName) SetContext(ctx context.Context, value models.ShipmentID) {
	tags.SetContext(ctx, tag.Setter(value))
}

type commandIDTagName string

func (tag commandIDTagName) Tag(value models.CommandID) opentracing.Tag {
	return opentracing.Tag{Key: string(tag), Value: value}
}

func (tag commandIDTagName) Set(span opentracing.Span, value models.CommandID) {
	span.SetTag(string(tag), value)
}

func (tag commandIDTagName) Setter(value models.CommandID) tags.Setter {
	return func(span opentracing.Span) {
		tag.Set(span, value)
	}
}

func (tag commandIDTagName) SetContext(ctx context.Context, value models.CommandID) {
	tags.SetContext(ctx, tag.Setter(value))
}

type jobIDTagName string

func (tag jobIDTagName) Tag(value models.JobID) opentracing.Tag {
	return opentracing.Tag{Key: string(tag), Value: value}
}

func (tag jobIDTagName) Set(span opentracing.Span, value models.JobID) {
	span.SetTag(string(tag), value)
}

func (tag jobIDTagName) Setter(value models.JobID) tags.Setter {
	return func(span opentracing.Span) {
		tag.Set(span, value)
	}
}

func (tag jobIDTagName) SetContext(ctx context.Context, value models.JobID) {
	tags.SetContext(ctx, tag.Setter(value))
}
