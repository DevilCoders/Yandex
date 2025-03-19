#!/usr/bin/env bash

prepare_envs_and_wd() {
  export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

  if [ -z "${NV_SYS_DIR_PREFIX}" ]; then
    log "Variable NV_SYS_DIR_PREFIX is not defined, using default"
    NV_SYS_DIR_PREFIX="sys/"
  fi
  if [ -z "${NV_LOG_DIR_PREFIX}" ]; then
    log "Variable NV_LOG_DIR_PREFIX is not defined, using default"
    NV_LOG_DIR_PREFIX="log/"
  fi

  NV_SYS_STATIC_DIR_PREFIX=${NV_SYS_DIR_PREFIX}static/
  # job_launcher Input
  JOB="${NV_SYS_DIR_PREFIX}job.json"
  JOB_SOURCE="$JOB"
  VM_OPTIONS_FILE="${NV_SYS_STATIC_DIR_PREFIX}job_launcher.vmoptions_short"
  # job_launcher Outputs
  OUTPUT="${NV_SYS_DIR_PREFIX}job_launcher.out.json"
  # Custom Job Status Messages
  ERROR_MSG="${NV_SYS_DIR_PREFIX}job_launcher.error_msg.txt"
  SUCCESS_MSG="${NV_SYS_DIR_PREFIX}job_launcher.success_msg.txt"
  # Logs
  PREPARE_LOG="${NV_LOG_DIR_PREFIX}job_launcher.diag.prepare.log"
  STDOUT_LOG="${NV_LOG_DIR_PREFIX}job_launcher.stdout.log"
  STDERR_LOG="${NV_LOG_DIR_PREFIX}job_launcher.stderr.log"
  HOOK_LOG="${NV_LOG_DIR_PREFIX}job_launcher.diag.hook.log"
  SLOT_FINISH_MARK=".nv-slot-finish.mark"


  mkdir -p "${NV_SYS_DIR_PREFIX}"
  mkdir -p "${NV_LOG_DIR_PREFIX}"

  if [ -e /dev/fd/5 ] ; then
    ln -s /dev/fd/5 ${NV_SYS_STATIC_DIR_PREFIX}static_fd_5
  else
    log "Descriptor number 5 does not exist"
  fi

  # JRE
  for dir in '/opt/nirvana-jre' '/place/db/iss3/jre' '/usr/lib/jvm/java-8-oracle/jre'; do
    if [ -d "$dir" ]; then
      JAVA_HOME="$dir"
      break
    fi
  done
  if [ -z "$JAVA_HOME" ]; then
    print_error_and_exit "No JDK or JRE found! Terminating."
  fi

  # remap secret envs (YT_SECURE_VAULT_MY_TOKEN -> MY_TOKEN)
  for varname in ${!YT_SECURE_VAULT_*}; do
      new_varname=${varname#YT_SECURE_VAULT_}
      export ${new_varname}=${!varname}
  done

  # job_launcher VM Options
  if [ "${NV_NATIVE_JOB_LAUNCHER}" = "true" ]; then
    VM_OPTIONS_FILTER='^#.*|^\s*$|^-XX.*'
  else
    VM_OPTIONS_FILTER='^#.*|^\s*$'
  fi
  VM_OPTIONS="$(cat ${VM_OPTIONS_FILE} | grep -E -v ${VM_OPTIONS_FILTER} | while read line; do echo $(eval echo `echo ${line}`); done)"
}

set_job_source() {
  if [ -z "${NV_JOB_JSON_URL}" ]; then
    if [ ! -f "${JOB}" ]; then
        print_error_and_exit "Neither job.json file nor download url was provided"
    fi
    JOB_SOURCE="$JOB"
  else
    JOB_SOURCE="$NV_JOB_JSON_URL"
  fi
}

log() {
  echo "$(date '+%Y-%m-%dT%H:%M:%S.%3N%:z') [$(basename -- "$0")] - $@" >>"$HOOK_LOG"
}

log_file_contents() {
  cat "$1" >>"$HOOK_LOG"
}

print_error_and_exit() {
  log "$@"
  echo >&2 '{"failure":"JOB_LAUNCHER/EXEC"}'
  if [ -f "$HOOK_LOG" ]; then
    echo >&2 'Hook log:'
    cat "$HOOK_LOG" >&2
  fi
  exit 1
}

run_vmtouch_async() {
  # async warmup page-cache
  if [ "${NV_SKIP_VMTOUCH}" != "true" ]; then
    vmtouch -l -f -m 10M -b ${NV_SYS_DIR_PREFIX}job_launcher_locked_files >/dev/null 2>&1 &
  fi
}

job_launcher() {
  local ret
  local oldifs

  log "starting job_launcher" "$@"
  oldifs="$IFS"
  IFS="$(printf '\n\t')"
  if [ "${NV_NATIVE_JOB_LAUNCHER}" = "true" ]; then
    /usr/bin/env ${NV_SYS_STATIC_DIR_PREFIX}job_launcher_native \
    -Djava.library.path=${JAVA_HOME}/lib/amd64 \
    -Djob.launcher.use.porto=false \
    -Dmds.amazon.s3.use.proxies=false \
    ${VM_OPTIONS} \
      -job "$JOB_SOURCE" \
      -o "$OUTPUT" \
      "$@" \
      >>"$PREPARE_LOG" 2>&1
  else
    /usr/bin/env bash ${NV_SYS_STATIC_DIR_PREFIX}jdkrun "${JAVA_HOME}" \
    java \
    ${VM_OPTIONS} \
    -jar ${NV_SYS_STATIC_DIR_PREFIX}job_launcher.jar \
      -job "$JOB_SOURCE" \
      -o "$OUTPUT" \
      "$@" \
      >>"$PREPARE_LOG" 2>&1
  fi
  ret="$?"
  IFS="$oldifs"

  if [ \( "$ret" -eq 1 \) -a \( ! -e "$OUTPUT" \) ]; then
    # No job_launcher output written, and retcode is 1.
    # This is most likely because job_launcher cannot be started:
    echo >"$OUTPUT" '{"failure":"JOB_LAUNCHER/EXEC"}'
    log '=> could not start job_launcher'
  else
    log "=> job_launcher exited with code ${ret}"
  fi

  return "$ret"
}

write_diagnostics() {
  log "job failed; collecting diagnostics:"

  declare -A diag_commands=( \
    ['ls.wd']='ls -lhAR' \
    ['dmesg']='dmesg -T | tail -n 100' \
    ['lsof']='lsof && echo "========" && lsof -i' \
    ['df']='df -h' \
    ['free']='free -t -m' \
    ['iostat']='iostat' \
    ['uptime']='uptime' \
    ['porto']='tail -n 1000 /var/log/portod.log' \
    ['dump.json']='cat dump.json' \
    ['mount']='mount'
  )

  log "  writing diagnostic logs:"
  for cmd in "${!diag_commands[@]}"; do
    diag_log_file="${NV_LOG_DIR_PREFIX}job_launcher.diag.${cmd}.log"
    log "    * $diag_log_file"
    eval "${diag_commands[$cmd]}" >"$diag_log_file" 2>/dev/null
  done
  log "  => finished writing diagnostic logs"

  log "=> dumping job_launcher stdout+stderr+PREPARE log:"
  log_file_contents "$PREPARE_LOG"

  log "=> finished collecting diagnostics"
}

write_output() {
  if [ -e "$OUTPUT" ]; then
    cat >&2 "$OUTPUT"
  elif [ -s "$PREPARE_LOG" ]; then
    cat >&2 "$PREPARE_LOG"
  fi
  echo >&2
}

save_logs() {
  local exit_code="$1"

  if [ "$exit_code" -ne 0 ]; then
    if [ -e "$ERROR_MSG" ]; then
      echo -n 'error message: ' >>"$STDERR_LOG"
      tail -n 1 "$ERROR_MSG" | tee -a "$STDERR_LOG"
    fi
    write_diagnostics
  else
    if [ -e "$SUCCESS_MSG" ]; then
      echo -n 'success message: ' >>"$STDOUT_LOG"
      tail -n 1 "$SUCCESS_MSG" | tee -a "$STDOUT_LOG"
    fi
  fi
  write_output

  job_launcher -e "$exit_code" -s SAVE_LOGS DEBUG
  return $?
}

run_job() {
  job_launcher PREPARE VALIDATE_COMMAND WAIT_FOR_HOSTS DOWNLOAD_INPUTS INSTALL_ENVIRONMENTS PREPARE_TRANSACTION LAUNCH RELEASE_HOSTS SAVE_RESULTS UNINSTALL_ENVIRONMENTS
  return $?
}

run_regular() {
  set_job_source
  run_job
  local exit_code=$?
  save_logs "${exit_code}"
  exit ${exit_code}
}

cleanup_after_slot() {
  sys_static_dir=${NV_SYS_STATIC_DIR_PREFIX%/} # to remove trailing slash
  find . -mindepth 1 -not \( -regex "^\./${sys_static_dir}\(/.*\)?" -or -name ".nv-slot-abort.mark" \) -and \( -type f -or -type l \) -delete
  find . -type d -empty -delete # if we run only previous command without "-type f" argument, we will get error like 'find: cannot delete `./nv_tmpfs/sys': Directory not empty'
  mkdir -p ${NV_LOG_DIR_PREFIX}
}

run_slot_cycle() {
  JOB_SOURCE=$JOB

  while true; do
    run_job
    local exit_code=$?
    if [ -e ${SLOT_FINISH_MARK} ]; then
      exit 0
    fi
    NV_SLOT_MODE=false save_logs ${exit_code}
    cleanup_after_slot
  done
}

jl_main() {
  run_vmtouch_async
  prepare_envs_and_wd

  if [ "${NV_SLOT_MODE}" = "true" ]; then
    run_slot_cycle
  else
    run_regular
  fi
}

