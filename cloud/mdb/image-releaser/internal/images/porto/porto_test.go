package porto

import (
	"context"
	"errors"
	"reflect"
	"testing"
	"time"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal/images"
	"a.yandex-team.ru/cloud/mdb/image-releaser/pkg/checker"
	"a.yandex-team.ru/cloud/mdb/image-releaser/pkg/checker/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	s3mocks "a.yandex-team.ru/cloud/mdb/internal/s3/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

var someTime = time.Date(2020, 03, 26, 0, 0, 0, 0, time.UTC)
var earlier = someTime.Add(-time.Hour)
var later = someTime.Add(time.Hour)

func Test_lastImage(t *testing.T) {
	type args struct {
		ctx             context.Context
		setExpectations func(m *s3mocks.MockClient)
		imageName       string
		bucket          string
	}
	var tests = []struct {
		name    string
		args    args
		want    s3ImgVersion
		wantErr bool
	}{
		{
			name: "happy path",
			args: args{
				setExpectations: func(m *s3mocks.MockClient) {
					m.EXPECT().ListObjects(gomock.Any(), "some_bucket", s3.ListObjectsOpts{
						Prefix: ptr.String("postgresql-bionic"),
					}).Return([]s3.Object{
						{
							Key:          "1",
							LastModified: someTime,
						},
						{
							Key:          "2",
							LastModified: later,
						},
						{
							Key:          "3",
							LastModified: earlier,
						},
					}, []s3.Prefix{}, nil)
				},
				imageName: "postgresql",
				bucket:    "some_bucket",
			},
			want: s3ImgVersion{
				objKey: "2",
				date:   later,
			},
			wantErr: false,
		},
		{
			name: "no images",
			args: args{
				setExpectations: func(m *s3mocks.MockClient) {
					m.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).
						Return([]s3.Object{}, []s3.Prefix{}, nil)
				},
				imageName: "postgresql",
				bucket:    "some_bucket",
			},
			want:    s3ImgVersion{},
			wantErr: true,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			m := s3mocks.NewMockClient(ctrl)
			if tt.args.setExpectations != nil {
				tt.args.setExpectations(m)
			}

			got, err := lastImage(tt.args.ctx, m, tt.args.imageName, tt.args.bucket)
			if (err != nil) != tt.wantErr {
				t.Errorf("lastImage() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if !reflect.DeepEqual(got, tt.want) {
				t.Errorf("lastImage() got = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestS3Releaser_Release(t *testing.T) {
	type fields struct {
		setExpectations   func(m *s3mocks.MockClient)
		fromBucket        string
		toBucket          string
		requiredStability time.Duration
	}
	type args struct {
		ctx       context.Context
		imageName string
		checkers  func(t *testing.T) []checker.Checker
	}
	const testBucket = "test_bucket"
	const prodBucket = "prod_bucket"
	tests := []struct {
		name    string
		fields  fields
		args    args
		wantErr error
	}{
		{
			name: "nothing to release: prod newer",
			fields: fields{
				setExpectations: func(m *s3mocks.MockClient) {
					m.EXPECT().ListObjects(gomock.Any(), testBucket, gomock.Any()).
						Return([]s3.Object{
							{
								Key:          "1",
								LastModified: someTime,
							},
						}, []s3.Prefix{}, nil)
					m.EXPECT().ListObjects(gomock.Any(), prodBucket, gomock.Any()).
						Return([]s3.Object{
							{
								Key:          "1",
								LastModified: later,
							},
						}, []s3.Prefix{}, nil)
				},
				fromBucket: testBucket,
				toBucket:   prodBucket,
			},
			args: args{
				imageName: "test_db",
				checkers: func(t *testing.T) []checker.Checker {
					mockChecker := mocks.NewMockChecker(gomock.NewController(t))
					mockChecker.EXPECT().IsStable(gomock.Any(), someTime, gomock.Any()).Times(1).Return(nil)
					return []checker.Checker{mockChecker}
				},
			},
			wantErr: images.ErrNothingReleased,
		},
		{
			name: "release - happy path",
			fields: fields{
				setExpectations: func(m *s3mocks.MockClient) {
					// list
					m.EXPECT().ListObjects(gomock.Any(), testBucket, gomock.Any()).
						Return([]s3.Object{
							{
								Key:          "newest",
								LastModified: someTime,
							},
						}, []s3.Prefix{}, nil)
					m.EXPECT().ListObjects(gomock.Any(), prodBucket, gomock.Any()).
						Return([]s3.Object{
							{
								Key:          "old",
								LastModified: earlier,
							},
						}, []s3.Prefix{}, nil)
					// copy
					m.EXPECT().CopyObject(gomock.Any(), testBucket, "newest", prodBucket, "newest").
						Return(nil)
				},
				fromBucket:        testBucket,
				toBucket:          prodBucket,
				requiredStability: time.Hour,
			},
			args: args{
				imageName: "test_db",
				checkers: func(t *testing.T) []checker.Checker {
					mockChecker := mocks.NewMockChecker(gomock.NewController(t))
					mockChecker.EXPECT().IsStable(gomock.Any(), someTime, gomock.Any()).Times(1).Return(nil)
					return []checker.Checker{mockChecker}
				},
			},
			wantErr: nil,
		},
		{
			name: "unstable",
			fields: fields{
				setExpectations: func(m *s3mocks.MockClient) {
					// list
					m.EXPECT().ListObjects(gomock.Any(), testBucket, gomock.Any()).
						Return([]s3.Object{
							{
								Key:          "newest",
								LastModified: someTime,
							},
						}, []s3.Prefix{}, nil)
				},
				fromBucket: testBucket,
				toBucket:   prodBucket,
			},
			args: args{
				imageName: "test_db",
				checkers: func(t *testing.T) []checker.Checker {
					mockChecker := mocks.NewMockChecker(gomock.NewController(t))
					mockChecker.EXPECT().IsStable(gomock.Any(), gomock.Any(), gomock.Any()).Times(1).Return(errors.New("unstable"))
					return []checker.Checker{mockChecker}
				},
			},
			wantErr: images.ErrUnstable,
		},
		{
			name: "first prod release",
			fields: fields{
				setExpectations: func(m *s3mocks.MockClient) {
					// list
					m.EXPECT().ListObjects(gomock.Any(), testBucket, gomock.Any()).
						Return([]s3.Object{
							{
								Key:          "newest",
								LastModified: someTime,
							},
						}, []s3.Prefix{}, nil)
					m.EXPECT().ListObjects(gomock.Any(), prodBucket, gomock.Any()).
						Return([]s3.Object{}, []s3.Prefix{}, nil)
					// copy
					m.EXPECT().CopyObject(gomock.Any(), testBucket, "newest", prodBucket, "newest").
						Return(nil)
				},
				fromBucket: testBucket,
				toBucket:   prodBucket,
			},
			args: args{
				imageName: "test_db",
				checkers: func(t *testing.T) []checker.Checker {
					mockChecker := mocks.NewMockChecker(gomock.NewController(t))
					mockChecker.EXPECT().IsStable(gomock.Any(), gomock.Any(), gomock.Any()).Times(1).Return(nil)
					return []checker.Checker{mockChecker}
				},
			},
			wantErr: nil,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			m := s3mocks.NewMockClient(ctrl)
			if tt.fields.setExpectations != nil {
				tt.fields.setExpectations(m)
			}

			s3r := &Porto{
				s3:                m,
				sourceBucket:      tt.fields.fromBucket,
				destinationBucket: tt.fields.toBucket,
			}
			if err := s3r.Release(tt.args.ctx, tt.args.imageName, tt.args.checkers(t)); (err != nil) && !xerrors.Is(err, tt.wantErr) {
				t.Errorf("Release() error = %v, wantErr %v", err, tt.wantErr)
			}
		})
	}
}
