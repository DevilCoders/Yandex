package juggler

import (
	"context"
	"errors"
	"testing"
	"time"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	jugglermock "a.yandex-team.ru/cloud/mdb/internal/juggler/mocks"
)

func TestJugglerChecker_IsStable(t *testing.T) {
	now := time.Date(2020, 03, 26, 0, 0, 0, 0, time.UTC)
	stableReqTime := time.Hour

	type fields struct {
		setExpectations func(j *jugglermock.MockAPI)
		checkHost       string
		checkService    string
		reqStability    time.Duration
	}
	type args struct {
		ctx   context.Context
		since time.Time
		now   time.Time
	}
	tests := []struct {
		name    string
		fields  fields
		args    args
		wantErr bool
	}{
		{
			name: "happy path",
			fields: fields{
				setExpectations: func(j *jugglermock.MockAPI) {
					j.EXPECT().GetChecksState(gomock.Any(), "postgresql-e2e.db.yandex.net", "dbaas-e2e-porto-qa-postgresql").
						Return([]juggler.CheckState{{
							ChangeTime: now.Add(-stableReqTime),
							Status:     "OK",
						}}, nil)
				},
				checkHost:    "postgresql-e2e.db.yandex.net",
				checkService: "dbaas-e2e-porto-qa-postgresql",
				reqStability: stableReqTime,
			},
			args: args{
				since: now.Add(-stableReqTime),
				now:   now,
			},
			wantErr: false,
		},
		{
			name: "too early to check",
			fields: fields{
				reqStability: stableReqTime,
			},
			args: args{
				since: now.Add(-stableReqTime / 2),
				now:   now,
			},
			wantErr: true,
		},
		{
			name: "e2e not enough stable time",
			fields: fields{
				setExpectations: func(j *jugglermock.MockAPI) {
					j.EXPECT().GetChecksState(gomock.Any(), gomock.Any(), gomock.Any()).
						Return([]juggler.CheckState{{
							ChangeTime: now.Add(-(stableReqTime) / 2), // not stable yet
							Status:     "OK",
						}}, nil)
				},
				reqStability: stableReqTime,
			},
			args: args{
				since: now.Add(-1000 * stableReqTime),
				now:   now,
			},
			wantErr: true,
		},
		{
			name: "juggler status ERROR",
			fields: fields{
				setExpectations: func(j *jugglermock.MockAPI) {
					j.EXPECT().GetChecksState(gomock.Any(), gomock.Any(), gomock.Any()).
						Return([]juggler.CheckState{{
							ChangeTime: now.Add(-(stableReqTime)),
							Status:     "ERROR",
						}}, nil)
				},
				reqStability: stableReqTime,
			},
			args: args{
				since: now.Add(-1000 * stableReqTime),
				now:   now,
			},
			wantErr: true,
		},
		{
			name: "juggler api error",
			fields: fields{
				setExpectations: func(j *jugglermock.MockAPI) {
					j.EXPECT().GetChecksState(gomock.Any(), gomock.Any(), gomock.Any()).
						Return(nil, errors.New("juggler_api_error"))
				},
				reqStability: stableReqTime,
			},
			args: args{
				since: now.Add(-1000 * stableReqTime),
				now:   now,
			},
			wantErr: true,
		},
		{
			name: "no juggler states",
			fields: fields{
				setExpectations: func(j *jugglermock.MockAPI) {
					j.EXPECT().GetChecksState(gomock.Any(), gomock.Any(), gomock.Any()).
						Return([]juggler.CheckState{}, nil)
				},
				reqStability: stableReqTime,
			},
			args: args{
				since: now.Add(-1000 * stableReqTime),
				now:   now,
			},
			wantErr: true,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			j := jugglermock.NewMockAPI(ctrl)
			if tt.fields.setExpectations != nil {
				tt.fields.setExpectations(j)
			}

			jc := &JugglerChecker{
				jugAPI:       j,
				checkHost:    tt.fields.checkHost,
				checkService: tt.fields.checkService,
				reqStability: tt.fields.reqStability,
			}
			if err := jc.IsStable(tt.args.ctx, tt.args.since, tt.args.now); (err != nil) != tt.wantErr {
				t.Errorf("IsStable() error = %v, wantErr %v", err, tt.wantErr)
			}
		})
	}
}
