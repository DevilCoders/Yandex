package internal

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"os"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (app *App) LogMetric(metric *Metric) error {
	app.logMutex.Lock()
	defer app.logMutex.Unlock()
	fh, err := os.OpenFile(app.Cfg.MetricsLogPath(), os.O_CREATE|os.O_APPEND, 0640)
	if err != nil {
		return xerrors.Errorf("failed to open metric log file: %w", err)
	}
	defer app.CloseLog(fh)
	_, err = fmt.Fprintln(fh, metric.String())
	if err != nil {
		return xerrors.Errorf("failed to write metric to file: %w", err)
	}
	return nil
}

func (app *App) SendMetricWithRetries(ctx context.Context, metric *Metric) bool {
	doc := writer.Doc{
		Data:      []byte(metric.String()),
		ID:        metric.SourceWT,
		CreatedAt: time.Unix(metric.SourceWT, 0),
	}
	for {
		_, err := writer.WriteAndWait(app.LbWriter, []writer.Doc{doc}, writer.FeedbackTimeout(app.Cfg.SendTimeout.Duration))
		if err == nil {
			app.L().Infof("metic %s sent to logbroker", metric.ID)
			return true
		}
		app.L().Errorf("failed to send metic %s to logbroker, retrying: %v", metric.ID, err)
		if !sleep(ctx, app.Cfg.RetryTimeout.Duration) {
			return false
		}
	}
}

func (app *App) NeedRotateLog() bool {
	stat, err := os.Stat(app.Cfg.MetricsLogPath())
	if err != nil {
		app.L().Errorf("failed to check metrics log size: %v", err)
		return false
	}
	return stat.Size() > app.Cfg.MaxMetricsLogSize
}

func (app *App) RotateLog() {
	app.logMutex.Lock()
	defer app.logMutex.Unlock()
	logPath := app.Cfg.MetricsLogPath()
	oldLogPath := logPath + ".old"
	err := os.Remove(oldLogPath)
	if err != nil && !os.IsNotExist(err) {
		app.L().Errorf("failed to remove old log file: %w", err)
		return
	}
	err = os.Rename(logPath, oldLogPath)
	if err != nil && !os.IsNotExist(err) {
		app.L().Errorf("failed to rename log file: %w", err)
		return
	}
	app.L().Infof("metrics log rotated")
}

func (app *App) SendLog(ctx context.Context, fh *os.File) error {
	sentState := app.GetSendState()
	scanner := bufio.NewScanner(fh)
	scanner.Split(bufio.ScanLines)
	for scanner.Scan() {
		data := scanner.Bytes()
		metric := new(Metric)
		err := json.Unmarshal(data, metric)
		if err != nil {
			app.L().Errorf("failed to unmarshall metric '%s': %w", string(data), err)
			continue
		}
		if metric.SourceWT <= sentState.SentTS {
			app.L().Debugf("metric %s skipped as sent", metric.ID)
			continue
		}
		sent := app.SendMetricWithRetries(ctx, metric)
		if sent {
			sentState.SentTS = metric.SourceWT
			err := app.SetSendState(sentState)
			if err != nil {
				app.L().Errorf("failed to update sent state: %v", err)
				app.L().Fatalf("halting to avoid overbilling")
			}
		}
		select {
		case <-ctx.Done():
			return nil
		default:
			continue
		}
	}
	return scanner.Err()
}

func (app *App) OpenLogWithRetries(ctx context.Context) *os.File {
	for {
		fh, err := os.OpenFile(app.Cfg.MetricsLogPath(), os.O_RDONLY|os.O_CREATE, 0640)
		if err == nil {
			app.L().Infof("metrics log file opened")
			return fh
		}
		app.L().Errorf("failed to open metrics log file: %s", err)
		if !sleep(ctx, app.Cfg.PollInterval.Duration) {
			return nil
		}
	}
}

func (app *App) CloseLog(fh *os.File) {
	if fh == nil {
		return
	}
	err := fh.Close()
	if err != nil {
		app.L().Warnf("failed to close log file: %v", err)
	}
}

func (app *App) Sender(ctx context.Context) {
	fh := app.OpenLogWithRetries(ctx)
	if fh == nil {
		return
	}
	defer app.CloseLog(fh)
	for {
		err := app.SendLog(ctx, fh)
		if err != nil {
			app.L().Errorf("failed to send log file: %v", err)
		}
		if app.NeedRotateLog() {
			app.CloseLog(fh)
			app.RotateLog()
			fh = app.OpenLogWithRetries(ctx)
			if fh == nil {
				return
			}
		}
		if !sleep(ctx, app.Cfg.PollInterval.Duration) {
			return
		}
	}
}

func sleep(ctx context.Context, d time.Duration) bool {
	t := time.NewTimer(d)
	defer t.Stop()
	select {
	case <-t.C:
		return true
	case <-ctx.Done():
		return false
	}
}
