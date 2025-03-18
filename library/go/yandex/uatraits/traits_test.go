package uatraits

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestTraits_IsMobile(t *testing.T) {
	type args struct {
		userAgent string
	}
	tests := []struct {
		name string
		args args
		want bool
	}{
		{
			name: "iphone",
			args: args{
				userAgent: "Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.1 Mobile/15E148 Safari/604.1",
			},
			want: true,
		},
		{
			name: "mac",
			args: args{
				userAgent: "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.135 YaBrowser/21.6.2.867 Yowser/2.5 Safari/537.36",
			},
			want: false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			detector, err := createDetector()
			require.NoError(t, err)
			traits := detector.Detect(tt.args.userAgent)

			got := traits.IsMobile()

			require.Equal(t, tt.want, got)
		})
	}
}

func TestTraits_IsTouch(t *testing.T) {
	type args struct {
		userAgent string
	}
	tests := []struct {
		name string
		args args
		want bool
	}{
		{
			name: "iphone",
			args: args{
				userAgent: "Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.1 Mobile/15E148 Safari/604.1",
			},
			want: true,
		},
		{
			name: "mac",
			args: args{
				userAgent: "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.135 YaBrowser/21.6.2.867 Yowser/2.5 Safari/537.36",
			},
			want: false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			detector, err := createDetector()
			require.NoError(t, err)
			traits := detector.Detect(tt.args.userAgent)

			got := traits.IsTouch()

			require.Equal(t, tt.want, got)
		})
	}
}

func TestTraits_IsMultiTouch(t *testing.T) {
	type args struct {
		userAgent string
	}
	tests := []struct {
		name string
		args args
		want bool
	}{
		{
			name: "iphone",
			args: args{
				userAgent: "Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.1 Mobile/15E148 Safari/604.1",
			},
			want: true,
		},
		{
			name: "mac",
			args: args{
				userAgent: "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.135 YaBrowser/21.6.2.867 Yowser/2.5 Safari/537.36",
			},
			want: false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			detector, err := createDetector()
			require.NoError(t, err)
			traits := detector.Detect(tt.args.userAgent)

			got := traits.IsMultiTouch()

			require.Equal(t, tt.want, got)
		})
	}
}

func TestTraits_IsTablet(t *testing.T) {
	type args struct {
		userAgent string
	}
	tests := []struct {
		name string
		args args
		want bool
	}{
		{
			name: "iPhone",
			args: args{
				userAgent: "Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.1 Mobile/15E148 Safari/604.1",
			},
			want: false,
		},
		{
			name: "mac",
			args: args{
				userAgent: "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.135 YaBrowser/21.6.2.867 Yowser/2.5 Safari/537.36",
			},
			want: false,
		},
		{
			name: "iPad",
			args: args{
				userAgent: "Mozilla/5.0 (iPad; CPU OS 12_5_4 like Mac OS X) WebKit/8607 (KHTML, like Gecko) Mobile/16H50 [FBAN/FBIOS;FBDV/iPad4,8;FBMD/iPad;FBSN/iOS;FBSV/12.5.4;FBSS/2;FBID/tablet;FBLC/it_IT;FBOP/5]",
			},
			want: true,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			detector, err := createDetector()
			require.NoError(t, err)
			traits := detector.Detect(tt.args.userAgent)

			got := traits.IsTablet()

			require.Equal(t, tt.want, got)
		})
	}
}

func TestTraits_Copy(t *testing.T) {
	tests := []struct {
		name string
		t    Traits
		want Traits
	}{
		{
			name: "nil traits",
			t:    nil,
			want: nil,
		},
		{
			name: "empty traits",
			t:    Traits{},
			want: Traits{},
		},
		{
			name: "not empty traits",
			t: Traits{
				"BrowserName": "Chrome",
				"IsMobile":    "true",
			},
			want: Traits{
				"BrowserName": "Chrome",
				"IsMobile":    "true",
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.want, tt.t.Copy())
		})
	}
}
