job("ch-backup-test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/ch-backup-test/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')

    parameters {
        stringParam('GERRIT_PATCHSET_REVISION', 'origin/master')
        stringParam('GERRIT_REFSPEC', '')
        stringParam('GERRIT_TOPIC', '')
    }

    scm {
        git {
            remote {
                name('origin')
                url('ssh://robot-pgaas-ci@review.db.yandex-team.ru:9440/mdb/ch-backup.git')
                credentials('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
                refspec('$GERRIT_REFSPEC')
            }
            branches('$GERRIT_PATCHSET_REVISION')
            extensions {
                cloneOptions {
                    timeout(300)
                }
            }
        }
    }

    triggers {
        gerrit {
            events{
                patchsetCreated()
                draftPublished()
            }
            project('mdb/ch-backup', 'reg_exp:.*')
        }
    }

    wrappers {
        timestamps()
        buildName('#${BUILD_NUMBER} ${ENV,var="GERRIT_TOPIC"}')
        colorizeOutput('xterm')
        credentialsBinding {
            string('ARCANUMAUTH', 'arcanum_token')
            string('YAV_OAUTH', 'robot-pgaas-ci-yav_oauth')
        }
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps {
        shell('env')
        shell('git log -1')
        shell('timeout 3600 make test')
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb/clickhouse')
        shell('''
python3 -c "import yaml; print('VERSIONS=' + ','.join(v['version'] for v in yaml.safe_load(open('arcadia/cloud/mdb/clickhouse/versions/versions.yaml'))))" > versions.properties
''')

        systemGroovyCommand('''
import jenkins.*
import jenkins.model.*
import hudson.*
import hudson.model.*

def build = Thread.currentThread().executable
def resolver = build.buildVariableResolver
def vars = build.getEnvVars()

def channel = Jenkins.getInstance().getComputer(vars["NODE_NAME"]).getChannel()
properties_file = new FilePath(channel, build.workspace.toString() + "/versions.properties")
properties = new Properties()
properties.load(properties_file.read())

def futures = []
for (ch_version in properties.VERSIONS.split(",")) {
  def job_name = "ch-backup-test-infra"
  println "Triggering downstream job $job_name for ClickHouse version $ch_version"
  def job = Hudson.instance.getJob(job_name)

  def params = [
    new StringParameterValue("GERRIT_TOPIC", resolver.resolve("GERRIT_TOPIC")),
    new StringParameterValue("GERRIT_REFSPEC", resolver.resolve("GERRIT_REFSPEC")),
    new StringParameterValue("GERRIT_PATCHSET_REVISION", resolver.resolve("GERRIT_PATCHSET_REVISION")),
    new StringParameterValue("CLICKHOUSE_VERSION", ch_version),
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
''') {sandbox(true)}
    }

    publishers {
        archiveArtifacts {
            pattern('staging/logs/**/*')
            allowEmpty(true)
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
