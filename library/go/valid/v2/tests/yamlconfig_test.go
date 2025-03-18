package config

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/httputil/headers"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/inspection"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

const (
	sizeSeparator rune = 'x'
)

type yamlConfig struct {
	Resizers []yamlResizer
	Storages []yamlStorage
	Buckets  []yamlBucket
}

func (y yamlConfig) Validate() error {
	return valid.Struct(&y,
		valid.Value(&y.Resizers, rule.NotEmpty, rule.Unique(func(v *inspection.Inspected) string {
			return v.Interface.(yamlResizer).Name
		})),
		valid.Value(&y.Storages, rule.NotEmpty, rule.Unique(func(v *inspection.Inspected) string {
			return v.Interface.(yamlStorage).Name
		})),
		valid.Value(&y.Buckets, rule.NotEmpty, rule.Unique(func(v *inspection.Inspected) string {
			buk := v.Interface.(yamlBucket)
			if buk.Target != "" {
				return buk.Target
			}
			return buk.Path
		})),
	)
}

type yamlResizer struct {
	Name           string
	BaseURL        string
	RequestTimeout time.Duration
	RetryCount     int
	Priority       int
}

func (y yamlResizer) Validate() error {
	return valid.Struct(&y,
		valid.Value(&y.Name, rule.NotEmpty, rule.IsASCII),
		valid.Value(&y.BaseURL, rule.NotEmpty, rule.IsURL),
		valid.Value(&y.RequestTimeout, rule.OmitEmpty(rule.IsPositive)),
		valid.Value(&y.RetryCount, rule.OmitEmpty(rule.IsPositive)),
		valid.Value(&y.Priority, rule.OmitEmpty(rule.IsPositive)),
	)
}

type yamlStorage struct {
	Name    string
	BaseURL string
}

func (y yamlStorage) Validate() error {
	return valid.Struct(&y,
		valid.Value(&y.Name, rule.NotEmpty, rule.IsASCII),
		valid.Value(&y.BaseURL, rule.NotEmpty, rule.IsURL),
	)
}

type yamlBucket struct {
	Path     string
	Target   string
	Storage  string
	Sizes    []string
	Aliases  map[string]string
	Features yamlFeatures
}

func (y yamlBucket) Validate() error {
	mAliasMessage := "must contain alias with key 'm'"

	return valid.Struct(&y,
		valid.Value(&y.Path, rule.NotEmpty, rule.IsASCII, rule.IsAbsDir),
		valid.Value(&y.Target, rule.OmitEmpty(rule.IsASCII, rule.IsAbsDir)),
		valid.Value(&y.Sizes, rule.NotEmpty, rule.Unique(rule.ValueAsKey), rule.Each(rule.Is2DMeasurements("x"))),
		valid.Value(&y.Aliases,
			rule.Message(mAliasMessage, rule.NotEmpty, rule.HasKey("m")),
			rule.Each(rule.Is2DMeasurements("x")),
		),
		valid.Value(&y.Features),
	)
}

type yamlFeatures struct {
	AllowProcessing     *bool
	BackgroundColor     *string
	ConvertTo           *string
	PreferType          *string
	ColorPalette        *uint8
	Watermark           *int
	DisableWatermarkFor []string
	Quality             *uint8
	ProxyExtensions     []string
	FallbackImage       *string
	EnableWMDK          *bool
}

func (y yamlFeatures) Validate() error {
	validImageType := rule.InSlice([]string{
		string(headers.TypeImageJPEG),
		string(headers.TypeImageGIF),
		string(headers.TypeImagePNG),
		string(headers.TypeImageWebP),
		string(headers.TypeImageSVG),
	})

	return valid.Struct(&y,
		valid.Value(&y.BackgroundColor, rule.OmitEmpty(rule.IsHexColor)),
		valid.Value(&y.ConvertTo, rule.OmitEmpty(validImageType)),
		valid.Value(&y.PreferType, rule.OmitEmpty(validImageType)),
		valid.Value(&y.ColorPalette, rule.OmitEmpty(rule.InRange(0, 128))),
		valid.Value(&y.Watermark, rule.OmitEmpty(rule.IsPositive)),
		valid.Value(&y.DisableWatermarkFor, rule.OmitEmpty(rule.Each(rule.NotEmpty))),
		valid.Value(&y.Quality, rule.OmitEmpty(rule.InRange(0, 100))),
		valid.Value(&y.ProxyExtensions, rule.OmitEmpty(validImageType)),
		valid.Value(&y.FallbackImage, rule.OmitEmpty(rule.IsURL)),
	)
}

func TestValidate_YamlConfig(t *testing.T) {
	t.Run("valid", func(t *testing.T) {
		c := yamlConfig{
			Resizers: []yamlResizer{
				{
					Name:           "shakalat",
					BaseURL:        "http://shakalat.local/",
					RequestTimeout: time.Second,
					RetryCount:     1,
					Priority:       100,
				},
				{
					Name:           "mds",
					BaseURL:        "http://resize.mds.local/",
					RequestTimeout: 2 * time.Second,
					RetryCount:     1,
					Priority:       10,
				},
			},
			Storages: []yamlStorage{
				{
					Name:    "s3",
					BaseURL: "http://s3.mds.yandex.ru/my-bucket/",
				},
			},
			Buckets: []yamlBucket{
				{
					Path:    "/test/valid_path/",
					Target:  "",
					Storage: "s3",
					Sizes: []string{
						"200x200",
						"400x400",
					},
					Aliases: map[string]string{
						"m": "200x200",
						"l": "400x400",
					},
					Features: yamlFeatures{
						BackgroundColor: ptr.String("FFFFFF"),
						PreferType:      ptr.String(string(headers.TypeImageWebP)),
						Quality:         ptr.Uint8(90),
					},
				},
			},
		}

		err := c.Validate()
		assert.NoError(t, err)
	})

	t.Run("empty_top_level", func(t *testing.T) {
		c := yamlConfig{
			Resizers: []yamlResizer{
				{
					Name:           "shakalat",
					BaseURL:        "http://shakalat.local/",
					RequestTimeout: time.Second,
					RetryCount:     1,
					Priority:       100,
				},
				{
					Name:           "мдс",
					BaseURL:        "http://resize.mds.local/",
					RequestTimeout: 2 * time.Second,
					RetryCount:     -1,
					Priority:       10,
				},
			},
			Storages: nil,
			Buckets: []yamlBucket{
				{
					Path:    "/test/valid_path",
					Target:  "",
					Storage: "s3",
					Sizes: []string{
						"200x200",
						"400x400",
					},
					Aliases: map[string]string{
						"l": "400x400",
					},
					Features: yamlFeatures{
						BackgroundColor: ptr.String("JKLMN"),
						PreferType:      ptr.String(string(headers.TypeImageWebP)),
						Quality:         ptr.Uint8(90),
					},
				},
			},
		}

		ic := inspection.Inspect(c)                      // inspected config
		ir := inspection.Inspect(c.Resizers[1])          // inspected resizer
		ib := inspection.Inspect(c.Buckets[0])           // inspected bucket
		ift := inspection.Inspect(c.Buckets[0].Features) // inspected feature

		expected := rule.Errors{
			rule.NewFieldError(
				&ic.Fields[0].Field,
				rule.NewFieldError(&ir.Fields[0].Field, rule.ErrInvalidCharacters),
			),
			rule.NewFieldError(
				&ic.Fields[0].Field,
				rule.NewFieldError(&ir.Fields[3].Field, rule.ErrNegativeValue),
			),
			rule.NewFieldError(&ic.Fields[1].Field, rule.ErrEmptyValue),
			rule.NewFieldError(
				&ic.Fields[2].Field,
				rule.NewFieldError(&ib.Fields[0].Field, rule.ErrPatternMismatch),
			),
			rule.NewFieldError(
				&ic.Fields[2].Field,
				rule.NewFieldError(&ib.Fields[4].Field, &rule.MessageErr{
					Msg: "must contain alias with key 'm'",
					Err: rule.ErrUnexpected,
				}),
			),
			rule.NewFieldError(
				&ic.Fields[2].Field,
				rule.NewFieldError(
					&ib.Fields[5].Field,
					rule.NewFieldError(&ift.Fields[1].Field, rule.ErrInvalidStringLength),
				),
			),
		}
		assert.Equal(t, expected, c.Validate())
	})
}

func BenchmarkValidate_YamlConfig(b *testing.B) {
	c := yamlConfig{
		Resizers: []yamlResizer{
			{
				Name:           "shakalat",
				BaseURL:        "http://shakalat.local/",
				RequestTimeout: time.Second,
				RetryCount:     1,
				Priority:       100,
			},
			{
				Name:           "mds",
				BaseURL:        "http://resize.mds.local/",
				RequestTimeout: 2 * time.Second,
				RetryCount:     1,
				Priority:       10,
			},
		},
		Storages: []yamlStorage{
			{
				Name:    "s3",
				BaseURL: "http://s3.mds.yandex.ru/my-bucket/",
			},
		},
		Buckets: []yamlBucket{
			{
				Path:    "/retailers/icons/",
				Target:  "",
				Storage: "s3",
				Sizes: []string{
					"100x100",
					"200x200",
					"400x400",
				},
				Aliases: map[string]string{
					"s": "100x100",
					"m": "200x200",
					"l": "400x400",
				},
				Features: yamlFeatures{
					PreferType: ptr.String(string(headers.TypeImageWebP)),
				},
			},
			{
				Path:    "/items/",
				Target:  "/offers/",
				Storage: "s3",
				Sizes: []string{
					"350x350",
					"450x450",
					"900x900",
				},
				Aliases: map[string]string{
					"s": "350x350",
					"m": "450x450",
					"l": "900x900",
				},
				Features: yamlFeatures{
					BackgroundColor:     ptr.String("FFFFFF"),
					ConvertTo:           ptr.String(string(headers.TypeImageJPEG)),
					Quality:             ptr.Uint8(90),
					DisableWatermarkFor: []string{".nwm"},
				},
			},
		},
	}

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = c.Validate()
	}
}
