package load

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"os"
	"sort"
	"strings"

	"golang.org/x/text/runes"
	"golang.org/x/text/transform"
	"golang.org/x/xerrors"
)

type ReadStat struct {
	Total           int64
	PostMethodCount int64
}

func Transform(inFileName, outFileName string) (*ReadStat, error) {
	var (
		inFile *os.File
		err    error
	)

	if inFileName == "" {
		inFile = os.Stdin
	} else {
		inFile, err := os.Open(inFileName)
		if err != nil {
			return nil, err
		}

		defer inFile.Close()
	}

	var outFile *os.File

	if outFileName == "" {
		outFile = os.Stdout
	} else {
		outFile, err = os.Open(outFileName)
		if err != nil {
			return nil, err
		}

		defer outFile.Close()
	}

	tr := transform.NewReader(inFile, runes.Map(
		func(r rune) rune {
			if r == '\'' {
				return '"'
			}

			return r
		},
	))

	dec := json.NewDecoder(tr)
	var stat ReadStat

	for {
		record, err := readRecord(dec, &stat)
		if xerrors.Is(err, io.EOF) {
			return &stat, nil
		}

		if err != nil {
			return nil, err
		}

		if record == nil {
			continue
		}

		sort.Slice(record.ProductIDs, func(i, j int) bool {
			return record.ProductIDs[i] < record.ProductIDs[j]
		})

		_, _ = fmt.Fprintf(outFile, "%s,%s", record.CloudID, strings.Join(record.ProductIDs, ";"))
	}
}

type requestBody struct {
	CloudID    string   `json:"cloud_id"`
	ProductIDs []string `json:"product_ids"`
}

func (r requestBody) str() string {
	b, err := json.Marshal(&r)
	if err != nil {
		log.Fatal(err)
	}

	return string(b)
}

func readRecord(dec *json.Decoder, stat *ReadStat) (*requestBody, error) {
	var body requestBody

	if err := dec.Decode(&body); err != nil {
		return nil, err
	}

	stat.PostMethodCount++

	return &body, nil
}
