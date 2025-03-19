package metriclabels

import "context"

type operationIDKey struct{}

type computeTaskIDKey struct{}

type taskIDKey struct{}

type formatKey struct{}

type useLabelKey struct{}

type Labels struct {
	DetailLabels bool

	OperationID   string
	ComputeTaskID string
	TaskID        string
	Format        string
}

func Get(ctx context.Context) (res Labels) {
	if val, ok := ctx.Value(operationIDKey{}).(string); ok {
		res.OperationID = val
	}

	if val, ok := ctx.Value(computeTaskIDKey{}).(string); ok {
		res.ComputeTaskID = val
	}

	if val, ok := ctx.Value(taskIDKey{}).(string); ok {
		res.TaskID = val
	}

	if val, ok := ctx.Value(formatKey{}).(string); ok {
		res.Format = val
	}

	if val, ok := ctx.Value(useLabelKey{}).(bool); ok {
		res.DetailLabels = val
	}

	return
}

func WithOperationID(ctx context.Context, opID string) context.Context {
	return context.WithValue(ctx, operationIDKey{}, opID)
}

func WithComputeTaskID(ctx context.Context, taskID string) context.Context {
	return context.WithValue(ctx, computeTaskIDKey{}, taskID)
}

func WithInternalTaskID(ctx context.Context, intTaskID string) context.Context {
	return context.WithValue(ctx, taskIDKey{}, intTaskID)
}

func WithFormat(ctx context.Context, format string) context.Context {
	return context.WithValue(ctx, formatKey{}, format)
}

func WithUseLabels(ctx context.Context, useLabels bool) context.Context {
	return context.WithValue(ctx, useLabelKey{}, useLabels)
}

func WithMetricData(ctx context.Context, data Labels) context.Context {
	ctx = WithOperationID(ctx, data.OperationID)
	ctx = WithComputeTaskID(ctx, data.ComputeTaskID)
	ctx = WithInternalTaskID(ctx, data.TaskID)
	ctx = WithFormat(ctx, data.Format)
	ctx = WithUseLabels(ctx, data.DetailLabels)
	return ctx
}
