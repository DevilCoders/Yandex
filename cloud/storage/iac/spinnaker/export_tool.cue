package spinnaker

args: {
	filepath: string | *"" @tag(filepath)
	dryRun:   bool | *true @tag(dryrun,type=bool)
}

// Export specified pipeline file to spinnaker
//
// Export specified pipeline file to spinnaker
//
// Filepath should be specified by tags, for example:
// cue cmd -t filepath=pipelines/default/bootstrap-base-role/bootstrap-salt-role.json export
// cue cmd -t filepath=pipelines/default/bootstrap-base-role/bootstrap-salt-role.json dryrun=false export
command: "export": {
	export: export_task & {
		#filepath: args.filepath
		#dryRun: args.dryRun
	}
}
