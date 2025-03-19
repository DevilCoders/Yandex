package images

import "testing"

func TestParseOS(t *testing.T) {
	tests := []struct {
		name    string
		arg     string
		want    OS
		wantErr bool
	}{
		{
			name:    "linux",
			arg:     "linux",
			want:    OSLinux,
			wantErr: false,
		},
		{
			name:    "windows",
			arg:     "windows",
			want:    OSWindows,
			wantErr: false,
		},
		{
			name:    "unknown",
			arg:     "foobar",
			want:    OSUnknown,
			wantErr: true,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := ParseOS(tt.arg)
			if (err != nil) != tt.wantErr {
				t.Errorf("ParseOS() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if got != tt.want {
				t.Errorf("ParseOS() got = %v, want %v", got, tt.want)
			}
		})
	}
}
