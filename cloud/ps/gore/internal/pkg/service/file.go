package service

type File struct {
	Format    string `json:"format,omitempty"`     // VERIFY: has date && has staff // "date:02/01/06,staff:primary,staff:backup",
	HasHeader *bool  `json:"has_header,omitempty"` // VERIFY: is ptr // "has_header": "false",
	Time      string `json:"time,omitempty"`       // VERIFY: 00-23:00-59 // "time": "12:00",
	URL       string `json:"url,omitempty"`        // VERIFY: get by url // "url": "https://bb.yandex-team.ru/projects/CLOUD/repos/resps/raw/cvs/yc_ps.csv"
}
