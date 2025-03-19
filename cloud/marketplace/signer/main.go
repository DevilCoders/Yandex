// signer is a cli that implements a client access to signer.yandex-team.ru v2 api
// as described here: https://wiki.yandex-team.ru/Intranet/signer/api_v2/

package main

import (
	"errors"
	"flag"
	"io"
	"log"
	"os"
	"path"
	"time"
)

var (
	app      = flag.String("app", "", "signer application name")
	token    = flag.String("token", "", "signer oauth token (https://oauth.yandex-team.ru/authorize?response_type=token&client_id=f2aac093416e4a949d727bd547207944)")
	input    = flag.String("input", "", "input file name")
	output   = flag.String("output", "", "output file name")
	endpoint = flag.String("endpoint", "https://signer.yandex-team.ru", "signer api endpoint")
)

func parseArgs() {
	flag.Parse()

	if flag.NFlag() < 4 {
		flag.Usage()
		os.Exit(2)
	}
}

func main() {
	parseArgs()

	// log line numbers for debugging purposes
	log.SetFlags(log.LstdFlags | log.Lshortfile)

	in, err := os.Open(*input)
	checkFatalErr("failed to open input file: %s", err)
	defer func() {
		// check if the input file is already closed
		// in most successful scenarios it will be closed by
		// http client after successful upload request
		if err := in.Close(); !errors.Is(err, os.ErrClosed) {
			checkFatalErr("failed to close input file: %s", err)
		}
	}()

	// we need to read input file info to create
	// output file with the same file mode
	inInfo, err := in.Stat()
	checkFatalErr("failed to read input file info: %s", err)

	signer := NewSigner(*app, *token, *endpoint)

	// create signature request for the input file
	_, inFilename := path.Split(*input)
	id, err := signer.CreateSignatureRequest(inFilename)
	checkFatalErr("failed to create signature request: %s", err)
	log.Printf("created signature request: %s", id)

	// upload file using created signature session
	// TODO(davidklassen): show progress indicator while uploading
	uploadRes, err := signer.UploadFile(id, in)
	checkFatalErr("failed to upload file: %s", err)
	log.Printf("uploaded input file sha1: %s", uploadRes.SHA1)

	for {
		// TODO(davidklassen):
		//   implement request backoff and deadline
		//   for signature status checks
		status, err := signer.CheckSignatureStatus(id)
		checkFatalErr("failed to check status: %s", err)
		log.Printf("check signature status: %s", status.Status)

		if status.Status == "complete" {
			log.Printf("signature complete, downloading file %s with sha1 %s", status.URL, status.SHA1)

			body, err := signer.RequestSignedFile(status.URL)
			checkFatalErr("failed to request signed file: %s", err)

			out, err := os.OpenFile(*output, os.O_RDWR|os.O_CREATE|os.O_TRUNC, inInfo.Mode())
			checkFatalErr("failed to open output file: %s", err)

			// TODO(davidklassen): compare sha1 checksum before writing to file
			_, err = io.Copy(out, body)
			checkFatalErr("failed to write downloaded file to output: %s", err)
			checkFatalErr("failed to close output file: %s", out.Close())
			checkErr("failed to close downloaded file body: %s", body.Close())
			log.Printf("saved signed file to %s", *output)
			break
		}

		time.Sleep(time.Second)
	}
}
