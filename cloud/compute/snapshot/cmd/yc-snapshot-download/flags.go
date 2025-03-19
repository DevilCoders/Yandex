package main

import "flag"

var (
	outFileparam        = flag.String("outfile", "", "'-' mean STDOUT. Default: <snapshotid>.raw (.raw.gz)")
	snapshotServerParam = flag.String("server", "localhost:7627", "GRPC endpoint of snapshot server")
	overwritePatam      = flag.Bool("overwrite", false, "Overwrite destination file if exist")
	compressionParam    = flag.String("compression", "", "Default - none. Allowed: gzip")
	showVersionParam    = flag.Bool("version", false, "show version and exit")
)
