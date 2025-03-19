#!/usr/bin/env bash

### Begin of Usage Message
usage() {
echo -e "Usage:
   ${0##*/} -h
   ${0##*/} [-f] [-l <logfile>] [-r <service>] <project> <keyfile> <ticket_dir>

   \e[1;3;32mTiny help for you:\e[0m
      -f - forced mode. Force key rotation and post-rotation actions.
           Do not verify digest at all.
      -h - print this help message and exit.
      -l - log file.
      -P - list possible project names.
      -R - rotate keys: current -> prev -> prevprev
      -r - service to make key reload. (default: reload_nginx ..., see code)
      -S - split key into 3 files: current, prev, prevprev

      \e[32m<project>\e[0m    project name.
                   Used to obtain key file for specific project.
                   List of projects available on ${SECHOST}:/repo/projects/
                   or by -P option

      \e[32m<keyfile>\e[0m    remote key file name.
                   Can be specified as file name or file path.
      \e[32m<ticket_dir>\e[0m directory to store tickets.

   Example:
   ~# ROBOT='robot-name' ${0##*/} httpbalancer yandex.tld /etc/ssl/tickets/
   HowTo:
      request tls tickets
         http://wiki/security/secdist#zakazkljuchejjdljatlstiketov
      put robot's private key in /home/$ROBOT/.ssh/id_rsa
      pubilc robot's key shold be on secdist:/home/$ROBOT/.ssh/authorized_keys
      run 3 times example above with -R
      configure nginx
         http://wiki/security/ssl/bestpractices#sslsessioncacheisessiontickets
   "
   exit 0;
} ### End of Usage Message

# Is required to stop script execution on unexpected command failures
set -e
set -u
#set -x

#### Begin of Global variables

: ${SECHOST:="secdist.yandex.net"}
: ${ROBOT:="robot-balancer"}
: ${PROJECTS_DIR:="/repo/projects"}
: ${KEY_SUFFIX:=".key"}
: ${DIGEST_SUFFIX:=".256digest"}
: ${TMPDIR:=/var/tmp/}
: ${SUDO=}
# ret codes
: ${NEED_UPDATE:=81}
: ${HAS_UPDATED:=82}

SSH_OPTOINS=" -q -l $ROBOT \
              -o ConnectionAttempts=4 \
              -o UserKnownHostsFile=/dev/null \
              -o StrictHostKeyChecking=no "


# Default configurations
LOGFILE="/var/log/get-secdist-keys.log"
log() {
   local cmd='dd conv=notrunc oflag=append of='
   if ${DEBUG:-false} || [[ "$*" =~ ^ERROR ]]; then
      cmd='tee -a '
   fi
   printf -- "[$(date "+%F %T")] $*\n"|$cmd$LOGFILE 2>/dev/null ||:
}

Service_reloadAction="reload_nginx"

#### End of Global variables

#### Parse opts
while getopts "dfhk:l:Pr:RSs:" opt; do
   case ${opt} in
      d) DEBUG=true ;;
      f) FORCE=true ;;
      h) usage; exit 0 ;;
      l) LOGFILE="${OPTARG%.log}.log" ;;
      P) $SUDO ssh $SSH_OPTOINS "$SECHOST" "ls /repo/projects/"; exit 0 ;;
      r) Service_reloadAction="$OPTARG" ;;
      R) ROTATE_KEYS=true ;;
      S) SPLIT_KEYS=true ;;
      s) SLEEP_ON_ERROR="$OPTARG" ;;
      *) echo "Unknown key. Run '$0 -h' for help"; exit 1 ;;
   esac
done
shift $((OPTIND-1))

if (( $# < 3 )); then    # Show usage when no arguments given
   echo -e "\e[31mERROR! not enough arguments\e[0m"
   usage
   exit 0
fi

if [ "$(id -un 2>/dev/null)" != "$ROBOT" ]; then
   : ${SUDO:=sudo -u $ROBOT}
   log "INFO: current user is not $ROBOT, use '$SUDO'"
fi

checkDigest() {
   local new_key=$1 local_key=$2 force=$3

   local new_digest=$(sha256sum "$new_key")
   local local_digest=$(sha256sum "$local_key" 2>/dev/null) # shutup if file missing

   if ${force:-false};then
      log "INFO: Force checkDigest to update keys"
      return $NEED_UPDATE
   else
      if [ "${new_digest% *}" != "${local_digest% *}" ]; then
         log "INFO: [checkDigest] digest local and remote file differ"
         return $NEED_UPDATE
      else
         return $HAS_UPDATED
      fi
   fi
}

# Compares stdin with <original> file. Returns 0 for equal data, 1 otherwise.
# Usage: cat <file1> <file2> ... | checkSplit <original>
checkSplit() { diff -q "$1" <( cat ) &>/dev/null; }
checkSize() { (( $(stat -c %s $1) == 48 )); }

getKeyFailsafe() {
   local src="$1" dst="$2" _i=0 rsync_log=""
   local from="${SECHOST}:${src}"
   if test -f $dst; then
      log "WARN: found stale tmp file, $(rm -vf $dst 2>&1)"
   fi

   until rsync_log="$(rsync -c -e "$SUDO ssh $SSH_OPTOINS" $from "$dst" 2>&1)"; do
      if ((++_i, _i>2)); then
         log "ERROR: getKeyFailsafe failed while copying keys from secdist"
         return 1
      fi
      log "WARN: getKey failed on retry #$_i: $rsync_log"
      sleep ${SLEEP_ON_ERROR:-$((RANDOM%30))};
   done
   return 0
}

extractSessionKey() {
   local keyfile="$1" keynum="$2"

   awk "BEGIN{k=0; p=0}
   /-----BEGIN SESSION TICKET KEY-----/{k++; p=1}
   k == $keynum && p{print}
   /-----END SESSION TICKET KEY-----/{p=0}" "$keyfile"
}

splitKeys() {
   local keyfile="$1"
   local key_1="${keyfile}.1" key_2="${keyfile}.2" key_3="${keyfile}.3"

   extractSessionKey "$keyfile" 1 > "$key_1"
   extractSessionKey "$keyfile" 2 > "$key_2"
   extractSessionKey "$keyfile" 3 > "$key_3"
   chmod_log=$(chmod -v 0440 $key_1 $key_2 $key_3 2>&1)
   if cat "$key_1" "$key_2" "$key_3"|checkSplit "$keyfile"; then
      log "INFO: Keyfile '$keyfile' has been successfully splitted. $chmod_log"
      return 0
   else
      log "ERROR: Failed to split keyfile '$keyfile'. Check permissions. $chmod_log"
      return 1
   fi
   return 0
}

updateKey() {
   local new="$1" current="$2"
   local prev="${current}.prev" prevprev="${current}.prevprev"
   local rotate_logs

   if checkSize $new ;then

      if ${SPLIT_KEYS:-false}; then
         if ! splitKeys  "$new"; then
            log "ERROR: splitKeys failed"
            return 1
         fi
      else
         # Write access to keyfile dir is required to move files
         if [ -w "${current%/*}" ]; then
            if ${ROTATE_KEYS:-false}; then
               rotate_logs[0]="Step one: $(cp -vaf $prev $prevprev 2>&1)"
               rotate_logs[1]="Step two: $(cp -vaf $current $prev 2>&1)"
               if rotate_logs[2]="Step three: $(mv -vf  $new $current 2>&1)"; then
                  chmod_log=$(chmod -v 0440 $current $prev $prevprev 2>&1)
                  log "INFO: Keyfile '$current' updated succeed."
                  log "INFO: Rotate log: ${rotate_logs[@]}, chmod log: $chmod_log"
               else
                  log "ERROR: Failed to update '$current', rotate log: ${rotate_logs[@]}"
                  return 1
               fi
            fi
         else
            log "WARN: directory ${current%/*} not wraitable for me $(id)"
            return 1
         fi
      fi
   else
      log "ERROR: key size not equal 48bytes!"
      return 1
   fi
}

on_update_success() {
   local reload_log key="$1"
   # Here you can specify actions, performed after successful key update
   # (update succeed && digest check succeed)
   if command -v "$Service_reloadAction" &>/dev/null; then
      if ! $Service_reloadAction $key;then
         log "ERROR: Service failed while reload key."
         return 1
      fi
   else
      log "ERROR: Unknown key reload script: '$Service_reloadAction'"
      return 1
   fi
}

on_update_fail() {
   # Here you can specify actions, performed after key update failure
   # (update succeed && digest check failed)
   return 0
}

# Code action for yout taste
reload_nothing() { return 0; }

reload_nginx() {    # Make nginx reload configuration to reload keys
   log "INFO: Making nginx to reload keys..."
   if nginx_reload_log=$(command service nginx reload 2>&1); then
      log "INFO: $nginx_reload_log"
   else
      log "ERROR: $nginx_reload_log"
   fi
}

if ! touch "${LOGFILE}"; then   # Check LOGFILE is writable
   log "WARN: Logfile is not writable"
fi

PROJECT="$1"
REMOTE_KEY_FILE="$PROJECTS_DIR/$PROJECT/${2%${KEY_SUFFIX}}$KEY_SUFFIX" # ensure suffix
LOCAL_KEY_FILE="${3%/}/${REMOTE_KEY_FILE##*/}"
REMOTE_DIGEST="${REMOTE_KEY_FILE}$DIGEST_SUFFIX"
TMP_KEY="${3%/}/tmp.key"
#trigger="${TMPDIR%/}/$0.trigger"

# Check availability of ${SECHOST}
if ! nc -z $SECHOST 22 &>/dev/null; then
   log "WARN: $SECHOST is unreachable."
   exit 1
fi


if getKeyFailsafe "$REMOTE_KEY_FILE" $TMP_KEY; then
   case $(checkDigest $TMP_KEY $LOCAL_KEY_FILE ${FORCE:-false} >&2; echo $?) in
      $NEED_UPDATE)
         if updateKey $TMP_KEY $LOCAL_KEY_FILE; then
            on_update_success $LOCAL_KEY_FILE
            log "INFO: Key update succeed."
            exit 0
         else
            log "WARN: Key update failed."
            on_update_fail
            exit 1
         fi
         ;;
      $HAS_UPDATED)
         log "INFO: Key has been checked successfully. No update required."
         rm -f $TMP_KEY
         exit 0
         ;;
      *)
         log "ERROR: Some thing went wrong... monitoring comming soon... "
         exit 1
      ;;
   esac
else
   log "WARN: Faled getKeyFailsafe, it is sad..."
fi
