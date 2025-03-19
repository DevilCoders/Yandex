job("ch-version-test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/ch-version-test/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')

    parameters {
        stringParam('CLICKHOUSE_VERSION', '')
    }

    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        timestamps()
        colorizeOutput('xterm')
        buildName('#${BUILD_NUMBER} ${CLICKHOUSE_VERSION}')
    }

    steps {
        shell('env')
        systemGroovyCommand('''
import jenkins.*
import jenkins.model.*
import hudson.*
import hudson.model.*

def build = Thread.currentThread().executable
def resolver = build.buildVariableResolver

def futures = []
for (job_name in ["arcadia-ch-tools-test-func", "ch-backup-test-infra"]) {
  println "Triggering downstream job $job_name"
  def job = Hudson.instance.getJob(job_name)

  def params = [
    new StringParameterValue("CLICKHOUSE_VERSION", resolver.resolve("CLICKHOUSE_VERSION")),
    new StringParameterValue("GERRIT_PATCHSET_REVISION", "origin/master"),
    new StringParameterValue("GERRIT_REFSPEC", ""),
    new StringParameterValue("GERRIT_TOPIC", ""),
    new StringParameterValue("ARCANUMID", "trunk"),
    new StringParameterValue("trigger_id", ""),
 ]

  futures << job.scheduleBuild2(0, new Cause.UpstreamCause(build), new ParametersAction(params))
}

println("Waiting for downstream jobs to complete")

should_wait = true

deadline = System.currentTimeMillis() + 14400000L

while (should_wait) {
  done_futures = 0
  for (future in futures) {
    if (future.isDone()) {
      job = future.get()
      if (job.result != Result.SUCCESS) {
        throw new AbortException("${job.getAbsoluteUrl()} failed")
      }
      done_futures += 1
    }
  }
  if (done_futures == futures.size()) {
    should_wait = false
  } else {
    if (System.currentTimeMillis() > deadline) {
      throw new AbortException("Deadline reached")
    }
    sleep 1000
  }
}
''') {sandbox(false)}
    }

    publishers {
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
