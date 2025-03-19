package spinnaker

args: {
	application: string | *"" @tag(application)
	pipeline: string | *"" @tag(pipeline)
}

// Import specified pipeline from specified application
//
// Import specified pipeline from specified application.
//
// Application and pipeline should be specified by tags, for example:
// cue cmd -t application=bootstrap-base-role -t pipeline=bootstrap-salt-role import
command: import: import_task & {
	#application: args.application
	#pipeline: args.pipeline
}
