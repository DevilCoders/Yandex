package compute

import (
	"context"
	"reflect"
	"testing"
	"time"

	"github.com/blang/semver/v4"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal/images"
	"a.yandex-team.ru/cloud/mdb/image-releaser/pkg/checker"
	checkerMocks "a.yandex-team.ru/cloud/mdb/image-releaser/pkg/checker/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	computeMocks "a.yandex-team.ru/cloud/mdb/internal/compute/compute/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/compute/operations"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	day                = time.Hour * 24
	now                = time.Now()
	later              = now.Add(time.Hour)
	yesterday          = now.Add(-day)
	dayBeforeYesterday = yesterday.Add(-day)
	tomorrow           = now.Add(day)
)

func Test_latestInFolder(t *testing.T) {
	type args struct {
		setExpectations func(mock *computeMocks.MockImageService)
		folder          string
		imageName       string
	}
	tests := []struct {
		name    string
		args    args
		want    compute.Image
		wantErr bool
	}{
		{
			name: "no next page",
			args: args{
				setExpectations: func(mock *computeMocks.MockImageService) {
					mock.EXPECT().List(gomock.Any(), "folder", compute.ListImagesOpts{}).Times(1).Return(compute.ListImagesResponse{
						Images: []compute.Image{
							{
								ID:        "2",
								Name:      "dbaas-redis-image-later",
								CreatedAt: later,
								Status:    readyStatus,
							},
							{
								ID:        "1",
								Name:      "dbaas-redis-image-now",
								CreatedAt: now,
								Status:    readyStatus,
							},
						},
						NextPageToken: "",
					}, nil)
				},
				folder:    "folder",
				imageName: "redis",
			},
			want: compute.Image{
				ID:        "2",
				CreatedAt: later,
				Name:      "dbaas-redis-image-later",
				Status:    readyStatus,
			},
			wantErr: false,
		},
		{
			name: "versioned later",
			args: args{
				setExpectations: func(mock *computeMocks.MockImageService) {
					mock.EXPECT().List(gomock.Any(), "folder", compute.ListImagesOpts{}).Times(1).Return(compute.ListImagesResponse{
						Images: []compute.Image{
							{
								ID:        "2",
								Name:      "dbaas-redis-image-later",
								CreatedAt: later,
								Status:    readyStatus,
							},
							{
								ID:        "1",
								Name:      "dbaas-redis-image-now",
								CreatedAt: now,
								Status:    readyStatus,
							},
							{
								ID:        "3",
								Name:      "dbaas-redis-4-image-latest",
								CreatedAt: later.Add(time.Hour),
								Status:    readyStatus,
							},
						},
						NextPageToken: "",
					}, nil)
				},
				folder:    "folder",
				imageName: "redis",
			},
			want: compute.Image{
				ID:        "2",
				CreatedAt: later,
				Name:      "dbaas-redis-image-later",
				Status:    readyStatus,
			},
			wantErr: false,
		},
		{
			name: "next page",
			args: args{
				setExpectations: func(mock *computeMocks.MockImageService) {
					mock.EXPECT().List(gomock.Any(), "folder", compute.ListImagesOpts{}).Times(1).Return(compute.ListImagesResponse{
						Images: []compute.Image{
							{
								ID:        "1",
								Name:      "dbaas-redis-image-now",
								CreatedAt: now,
								Status:    readyStatus,
							},
						},
						NextPageToken: "nextPage",
					}, nil)
					mock.EXPECT().List(gomock.Any(), "folder", compute.ListImagesOpts{
						PageToken: "nextPage",
					}).Times(1).Return(compute.ListImagesResponse{
						Images: []compute.Image{
							{
								ID:        "2",
								Name:      "dbaas-redis-image-later",
								CreatedAt: later,
								Status:    readyStatus,
							},
						},
						NextPageToken: "",
					}, nil)
				},
				folder:    "folder",
				imageName: "redis",
			},
			want: compute.Image{
				ID:        "2",
				CreatedAt: later,
				Name:      "dbaas-redis-image-later",
				Status:    readyStatus,
			},
			wantErr: false,
		},
		{
			name: "filter not ready images",
			args: args{
				setExpectations: func(mock *computeMocks.MockImageService) {
					mock.EXPECT().List(gomock.Any(), "folder", compute.ListImagesOpts{}).Times(1).Return(compute.ListImagesResponse{
						Images: []compute.Image{
							{
								ID:        "2",
								Name:      "dbaas-redis-image-later",
								CreatedAt: later,
								Status:    creatingStatus,
							},
							{
								ID:        "1",
								Name:      "dbaas-redis-image-now",
								CreatedAt: now,
								Status:    readyStatus,
							},
						},
						NextPageToken: "",
					}, nil)
				},
				folder:    "folder",
				imageName: "redis",
			},
			want: compute.Image{
				ID:        "1",
				CreatedAt: now,
				Name:      "dbaas-redis-image-now",
				Status:    readyStatus,
			},
			wantErr: false,
		},
		{
			name: "filter other db images",
			args: args{
				setExpectations: func(mock *computeMocks.MockImageService) {
					mock.EXPECT().List(gomock.Any(), "folder", compute.ListImagesOpts{}).Times(1).Return(compute.ListImagesResponse{
						Images: []compute.Image{
							{
								ID:        "2",
								Name:      "dbaas-postgresql-later",
								CreatedAt: later,
								Status:    readyStatus,
							},
							{
								ID:        "1",
								Name:      "dbaas-redis-image-now",
								CreatedAt: now,
								Status:    readyStatus,
							},
						},
						NextPageToken: "",
					}, nil)
				},
				folder:    "folder",
				imageName: "redis",
			},
			want: compute.Image{
				ID:        "1",
				CreatedAt: now,
				Name:      "dbaas-redis-image-now",
				Status:    readyStatus,
			},
			wantErr: false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			mock := computeMocks.NewMockImageService(ctrl)
			if tt.args.setExpectations != nil {
				tt.args.setExpectations(mock)
			}
			got, err := latestInFolder(context.Background(), mock, tt.args.folder, tt.args.imageName)
			if (err != nil) != tt.wantErr {
				t.Errorf("latestInFolder() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if !reflect.DeepEqual(got, tt.want) {
				t.Errorf("latestInFolder() got = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestCompute_Release(t *testing.T) {
	type fields struct {
		s3urlPrefix  string
		sourceFolder string
	}
	type args struct {
		imageName  string
		poolSize   int
		folderID   string
		os         images.OS
		productIDs []string
	}
	tests := []struct {
		name            string
		fields          fields
		args            args
		setExpectations func(preprodImg *computeMocks.MockImageService,
			prodImg *computeMocks.MockImageService, prodOp *computeMocks.MockOperationService)
		checkers func(t *testing.T) []checker.Checker
		wantErr  bool
	}{
		{
			name: "happy path - release new",
			fields: fields{
				s3urlPrefix:  "https://s3/bucket",
				sourceFolder: "preprodFolder",
			},
			args: args{
				imageName:  "redis",
				folderID:   "prodFolder",
				poolSize:   5,
				os:         images.OSLinux,
				productIDs: []string{"iddqd", "idkfa"},
			},
			setExpectations: func(preprodImg *computeMocks.MockImageService,
				prodImg *computeMocks.MockImageService, prodOp *computeMocks.MockOperationService) {
				// preprod READY image
				preprodImg.EXPECT().List(gomock.Any(), "preprodFolder", gomock.Any()).Times(1).Return(compute.ListImagesResponse{
					Images: []compute.Image{
						{
							ID:        "1",
							Name:      "dbaas-redis-image-now",
							CreatedAt: now,
							Status:    readyStatus,
						},
					},
				}, nil)

				// prod no image
				noImgCall := prodImg.EXPECT().List(gomock.Any(), "prodFolder", compute.ListImagesOpts{Name: "dbaas-redis-image-now"}).Times(1).
					Return(compute.ListImagesResponse{Images: []compute.Image{}}, nil)
				// prod create image
				prodImg.EXPECT().Create(gomock.Any(), "prodFolder", "dbaas-redis-image-now",
					compute.CreateImageSourceURI("https://s3/bucket/dbaas-redis-image-now.img"), compute.OSLinux, gomock.Any()).Times(1).Return(
					operations.Operation{ID: "createOpID"}, nil)
				// wait for create operation to complete
				prodOp.EXPECT().GetDone(gomock.Any(), "createOpID").Times(1).Return(operations.Operation{
					Done:   true,
					Result: "success",
				}, nil)
				// return created image
				imgCreatedCall := prodImg.EXPECT().List(gomock.Any(), "prodFolder", compute.ListImagesOpts{Name: "dbaas-redis-image-now"}).Times(1).
					Return(compute.ListImagesResponse{Images: []compute.Image{{
						ID:   "prodImageID",
						Name: "dbaas-redis-image-now",
					}}}, nil)

				gomock.InOrder(
					noImgCall,
					imgCreatedCall,
				)
			},
			checkers: func(t *testing.T) []checker.Checker {
				ctrl := gomock.NewController(t)
				chmock := checkerMocks.NewMockChecker(ctrl)
				// second parameter is important - it should be the same as preprod image creation
				chmock.EXPECT().IsStable(gomock.Any(), now, gomock.Any()).Times(1).Return(nil)
				return []checker.Checker{chmock}
			},
			wantErr: false,
		},
		{
			name: "image is creating",
			fields: fields{
				s3urlPrefix:  "https://s3/bucket",
				sourceFolder: "preprodFolder",
			},
			args: args{
				imageName: "redis",
				folderID:  "prodFolder",
				poolSize:  5,
			},
			setExpectations: func(preprodImg *computeMocks.MockImageService,
				prodImg *computeMocks.MockImageService, prodOp *computeMocks.MockOperationService) {
				// preprod READY image
				preprodImg.EXPECT().List(gomock.Any(), "preprodFolder", gomock.Any()).Times(1).Return(compute.ListImagesResponse{
					Images: []compute.Image{
						{
							ID:        "1",
							Name:      "dbaas-redis-image-now",
							CreatedAt: now,
							Status:    readyStatus,
						},
					},
				}, nil)

				// prod image is creating
				prodImg.EXPECT().List(gomock.Any(), "prodFolder", compute.ListImagesOpts{Name: "dbaas-redis-image-now"}).Times(1).
					Return(compute.ListImagesResponse{Images: []compute.Image{{
						ID:     "prodImageID",
						Name:   "dbaas-redis-image-now",
						Status: creatingStatus,
					}}}, nil)
				// prod get creation operation
				prodImg.EXPECT().ListOperations(gomock.Any(), compute.ListOperationsOpts{ImageID: "prodImageID"}).Times(1).Return(
					[]operations.Operation{{
						ID: "createOpID",
					}}, nil)
				// wait for create operation to complete
				prodOp.EXPECT().GetDone(gomock.Any(), "createOpID").Times(1).Return(operations.Operation{
					Done:   true,
					Result: "success",
				}, nil)
			},
			checkers: func(t *testing.T) []checker.Checker {
				ctrl := gomock.NewController(t)
				chmock := checkerMocks.NewMockChecker(ctrl)
				chmock.EXPECT().IsStable(gomock.Any(), gomock.Any(), gomock.Any()).Times(1).Return(nil)
				return []checker.Checker{chmock}
			},
			wantErr: false,
		},
		{
			name: "image is present, but no pool",
			fields: fields{
				s3urlPrefix:  "https://s3/bucket",
				sourceFolder: "preprodFolder",
			},
			args: args{
				imageName: "redis",
				folderID:  "prodFolder",
				poolSize:  5,
			},
			setExpectations: func(preprodImg *computeMocks.MockImageService,
				prodImg *computeMocks.MockImageService, prodOp *computeMocks.MockOperationService) {
				// preprod READY image
				preprodImg.EXPECT().List(gomock.Any(), "preprodFolder", gomock.Any()).Times(1).Return(compute.ListImagesResponse{
					Images: []compute.Image{
						{
							ID:        "1",
							Name:      "dbaas-redis-image-now",
							CreatedAt: now,
							Status:    readyStatus,
						},
					},
				}, nil)

				// prod image is READY
				prodImg.EXPECT().List(gomock.Any(), "prodFolder", compute.ListImagesOpts{Name: "dbaas-redis-image-now"}).Times(1).
					Return(compute.ListImagesResponse{Images: []compute.Image{{
						ID:     "prodImageID",
						Name:   "dbaas-redis-image-now",
						Status: readyStatus,
					}}}, nil)
			},
			checkers: func(t *testing.T) []checker.Checker {
				ctrl := gomock.NewController(t)
				chmock := checkerMocks.NewMockChecker(ctrl)
				chmock.EXPECT().IsStable(gomock.Any(), gomock.Any(), gomock.Any()).Times(1).Return(nil)
				return []checker.Checker{chmock}
			},
			wantErr: false,
		},
		{
			name: "everything is present",
			fields: fields{
				s3urlPrefix:  "https://s3/bucket",
				sourceFolder: "preprodFolder",
			},
			args: args{
				imageName: "redis",
				folderID:  "prodFolder",
				poolSize:  5,
			},
			setExpectations: func(preprodImg *computeMocks.MockImageService,
				prodImg *computeMocks.MockImageService, prodOp *computeMocks.MockOperationService) {

				// preprod READY image
				preprodImg.EXPECT().List(gomock.Any(), "preprodFolder", gomock.Any()).Times(1).Return(compute.ListImagesResponse{
					Images: []compute.Image{
						{
							ID:        "1",
							Name:      "dbaas-redis-image-now",
							CreatedAt: now,
							Status:    readyStatus,
						},
					},
				}, nil)

				// prod image is READY
				prodImg.EXPECT().List(gomock.Any(), "prodFolder", compute.ListImagesOpts{Name: "dbaas-redis-image-now"}).Times(1).
					Return(compute.ListImagesResponse{Images: []compute.Image{{
						ID:     "prodImageID",
						Name:   "dbaas-redis-image-now",
						Status: readyStatus,
					}}}, nil)
			},
			checkers: func(t *testing.T) []checker.Checker {
				ctrl := gomock.NewController(t)
				chmock := checkerMocks.NewMockChecker(ctrl)
				chmock.EXPECT().IsStable(gomock.Any(), gomock.Any(), gomock.Any()).Times(1).Return(nil)
				return []checker.Checker{chmock}
			},
			wantErr: false,
		},
		{
			name: "image is not ready",
			fields: fields{
				s3urlPrefix:  "https://s3/bucket",
				sourceFolder: "preprodFolder",
			},
			args: args{
				imageName: "redis",
				folderID:  "prodFolder",
				poolSize:  5,
			},
			setExpectations: func(preprodImg *computeMocks.MockImageService,
				prodImg *computeMocks.MockImageService, prodOp *computeMocks.MockOperationService) {
				// preprod READY image
				preprodImg.EXPECT().List(gomock.Any(), "preprodFolder", gomock.Any()).Times(1).Return(compute.ListImagesResponse{
					Images: []compute.Image{
						{
							ID:        "1",
							Name:      "dbaas-redis-image-now",
							CreatedAt: now,
							Status:    readyStatus,
						},
					},
				}, nil)
			},
			checkers: func(t *testing.T) []checker.Checker {
				ctrl := gomock.NewController(t)
				chmock := checkerMocks.NewMockChecker(ctrl)
				chmock.EXPECT().IsStable(gomock.Any(), gomock.Any(), gomock.Any()).Times(1).Return(xerrors.New("image is not ready"))
				return []checker.Checker{chmock}
			},
			wantErr: true,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			c := &Compute{
				s3urlPrefix: tt.fields.s3urlPrefix,
				l:           &nop.Logger{},
			}
			ctrl := gomock.NewController(t)
			preprodImg := computeMocks.NewMockImageService(ctrl)
			c.srcImg = &computeProvider{preprodImgAPI: preprodImg, preprodFolder: tt.fields.sourceFolder}
			prodImg := computeMocks.NewMockImageService(ctrl)
			c.destinationImgAPI = prodImg
			prodOp := computeMocks.NewMockOperationService(ctrl)
			c.destinationOperationAPI = prodOp

			if tt.setExpectations != nil {
				tt.setExpectations(preprodImg, prodImg, prodOp)
			}
			if err := c.Release(context.Background(), tt.args.imageName, tt.args.os, tt.args.productIDs, tt.args.poolSize, tt.args.folderID, tt.checkers(t)); (err != nil) != tt.wantErr {
				t.Errorf("Release() error = %v, wantErr %v", err, tt.wantErr)
			}
		})
	}
}

func Test_takeOldestAfterN(t *testing.T) {
	tests := []struct {
		name   string
		images []compute.Image
		want   []compute.Image
	}{
		{
			"There less then n images",
			[]compute.Image{
				{Name: "yesterday", CreatedAt: yesterday},
				{Name: "now", CreatedAt: now},
			},
			nil,
		},
		{
			"There are only n images",
			[]compute.Image{
				{Name: "now", CreatedAt: now},
				{Name: "yesterday", CreatedAt: yesterday},
				{Name: "tomorrow", CreatedAt: tomorrow},
			},
			nil,
		},
		{
			"There are only n images",
			[]compute.Image{
				{Name: "tomorrow", CreatedAt: tomorrow},
				{Name: "yesterday", CreatedAt: yesterday},
				{Name: "now", CreatedAt: now},
				{Name: "dayBeforeYesterday", CreatedAt: dayBeforeYesterday},
			},
			[]compute.Image{
				{Name: "dayBeforeYesterday", CreatedAt: dayBeforeYesterday},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := takeOldestAfterN(tt.images, 3); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("takeOldestAfterN() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestCompute_Cleanup(t *testing.T) {
	tests := []struct {
		name     string
		testCase func(*testing.T, *Compute, *computeMocks.MockImageService)
	}{
		{
			"ERROR and old images",
			func(t *testing.T, c *Compute, prodImg *computeMocks.MockImageService) {
				prodImg.EXPECT().List(gomock.Any(), "prodFolder", compute.ListImagesOpts{}).Times(1).
					Return(compute.ListImagesResponse{Images: []compute.Image{
						{
							ID:        "CreatingID",
							Name:      "dbaas-redis-image-date",
							Status:    creatingStatus,
							CreatedAt: tomorrow,
						},
						{
							ID:        "Created-Now-ID",
							Name:      "dbaas-redis-image-date",
							Status:    readyStatus,
							CreatedAt: now,
						},
						{
							ID:        "Failed-Now-ID",
							Name:      "dbaas-redis-image-date",
							Status:    errorStatus,
							CreatedAt: now,
						},
						{
							ID:        "Created-Yesterday-ID",
							Name:      "dbaas-redis-image-date",
							Status:    readyStatus,
							CreatedAt: yesterday,
						},
						{
							ID:        "Failed-Yesterday-ID",
							Name:      "dbaas-redis-image-date",
							Status:    errorStatus,
							CreatedAt: yesterday,
						},
						{
							ID:        "Created-At-Day-Before-Yesterday-ID",
							Name:      "dbaas-redis-image-date",
							Status:    readyStatus,
							CreatedAt: dayBeforeYesterday,
						},
					}}, nil)

				for _, id := range []string{
					"Failed-Now-ID", "Failed-Yesterday-ID", "Created-At-Day-Before-Yesterday-ID",
				} {
					prodImg.EXPECT().Delete(gomock.Any(), id).Times(1)
				}

				require.NoError(t, c.Cleanup(context.Background(), "redis", 2, "prodFolder"))
			},
		},
		{
			"return errors in image delete, but try other deletes",
			func(t *testing.T, c *Compute, prodImg *computeMocks.MockImageService) {
				prodImg.EXPECT().List(gomock.Any(), "prodFolder", compute.ListImagesOpts{}).Times(1).
					Return(compute.ListImagesResponse{Images: []compute.Image{
						{
							ID:        "Created-Now-ID",
							Name:      "dbaas-redis-image-date",
							Status:    readyStatus,
							CreatedAt: now,
						},
						{
							ID:        "Failed-Now-ID",
							Name:      "dbaas-redis-image-date",
							Status:    errorStatus,
							CreatedAt: now,
						},
						{
							ID:        "Failed-Yesterday-ID",
							Name:      "dbaas-redis-image-date",
							Status:    errorStatus,
							CreatedAt: yesterday,
						},
					}}, nil)

				prodImg.EXPECT().Delete(gomock.Any(), "Failed-Now-ID").Times(1).Return(
					operations.Operation{},
					xerrors.New("image delete error"),
				)
				prodImg.EXPECT().Delete(gomock.Any(), "Failed-Yesterday-ID").Times(1)

				require.Error(t, c.Cleanup(context.Background(), "redis", 2, "prodFolder"))
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			c := &Compute{
				l: &nop.Logger{},
			}
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			preprodImg := computeMocks.NewMockImageService(ctrl)
			c.srcImg = &computeProvider{preprodImgAPI: preprodImg, preprodFolder: "preprodFolder"}
			prodImg := computeMocks.NewMockImageService(ctrl)
			c.destinationImgAPI = prodImg

			tt.testCase(t, c, prodImg)
		})
	}
}

func TestGetLatestVersion(t *testing.T) {
	testImages := []compute.Image{
		{ID: "image_for_version_2.0.1", Family: "dataproc-image-2-0", Labels: map[string]string{"version": "2.0.1"}},
		{ID: "image_for_version_2.0.2", Family: "dataproc-image-2-0", Labels: map[string]string{"version": "2.0.2"}},
		{ID: "image_for_version_2.1.1", Family: "dataproc-image-2-0", Labels: map[string]string{"version": "2.1.1"}},
		{ID: "image_for_version_1.4.0", Family: "dataproc-image-1-4", Labels: map[string]string{"version": "1.4.0"}},
		{ID: "image_for_version_1.4.1", Family: "dataproc-image-1-4", Labels: map[string]string{"version": "1.4.1"}},
		{ID: "image_for_version_3.0.0", Family: "dataproc-image-3-0", Labels: map[string]string{"version": "3.0.0"}},
	}
	version, _ := semver.Make("3.0.0")
	require.Equal(t, getLatestVersion(testImages, "", ""), version)

	version, _ = semver.Make("1.4.1")
	require.Equal(t, getLatestVersion(testImages, "", "dataproc-image-1-4"), version)

	version, _ = semver.Make("1.4.0")
	require.Equal(t, getLatestVersion(testImages, "1.4.0", "dataproc-image-1-4"), version)

	version, _ = semver.Make("1.4.1")
	require.Equal(t, getLatestVersion(testImages, "1.4", "dataproc-image-1-4"), version)

	version, _ = semver.Make("2.1.1")
	require.Equal(t, getLatestVersion(testImages, "2", ""), version)

	version, _ = semver.Make("2.0.2")
	require.Equal(t, getLatestVersion(testImages, "2.0", ""), version)
}
