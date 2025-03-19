#!/usr/bin/env bash

### Begin of Usage Message
usage() {
echo "Usage:
   ${0##*/} -h
   ${0##*/} [-f] [-l <logfile>] [-r <service>] <project> <keyfile> <ticket_dir>

   [1;3;32mTiny help for you:[0m
      -f - forced mode. Force key rotation and post-rotation actions.
           Do not verify digest at all.
      -h - print this help message and exit.
      -l - log file.
      -P - list possible project names.
      -R - rotate keys: current -> prev -> prevprev
      -r - service to make key reload. (reload_nginx ..., see code)
      -S - split key into 3 files: current, prev, prevprev

      [32m<project>[0m    project name.
                   Used to obtain key file for specific project.
                   List of projects available on ${SECHOST}:/repo/projects/
                   or by -P option

      [32m<keyfile>[0m    remote key file name.
                   Can be specified as file name or file path.
      [32m<ticket_dir>[0m directory to store tickets.

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
} ### End of Usage Message

# Is required to stop script execution on unexpected command failures
set -e
set -u
#set -x

#### Begin of Global variables

: ${SECHOST:="secdist.yandex.net"}
: ${ROBOT:="robot-media-salt"}
: ${PROJECTS_DIR:="/repo/projects"}
: ${KEY_SUFFIX:=".key"}
: ${DIGEST_SUFFIX:=".256digest"}
: ${TMPDIR:=/tmp/}
: ${SUDO=}
ssh_optoins="-o ConnectionAttempts=4 -o UserKnownHostsFile=/dev/null \
 -o StrictHostKeyChecking=no -l $ROBOT"

MD5="$(command -v shasum)"
if [ "${?}" -ne 0 ]; then
   echo "Error: shasum binary file not found"
   exit 1
fi

# Default configurations
logfile="/var/log/get-secdist-keys.log"
log_action() { printf -- "[$(date "+%F %T")] $*\n"|tee -a "$logfile"||: ; }

service_reloadAction="reload_nothing"

#### End of Global variables

#### Parse opts
while getopts "fhk:l:Pr:RSs:" opt; do
   case ${opt} in
      f) force=true
         ;;
      h) usage; exit 0
         ;;
      l) logfile="${OPTARG%.log}.log"
         ;;
      P) $SUDO ssh $ssh_optoins "$SECHOST" "ls /repo/projects/"
         exit 0
         ;;
      r) service_reloadAction="$OPTARG"
         ;;
      R) rotate_keys=true
         ;;
      S) split_keys=true
         ;;
      s) sleep_on_error="$OPTARG"
         ;;
      *) echo "Unknown key. Run '$0 -h' for help"
         exit 1
         ;;
   esac
done
shift $(($OPTIND - 1))

if [ ${#} -ne 3 ]; then    # Show usage when no arguments given
   echo "[31mERROR! not enough arguments[0m"
   usage; exit 0
fi

if [ "$(id -un 2>/dev/null)" != "$ROBOT" ]; then
   : ${SUDO:=sudo -u $ROBOT}
   log_action "WARNING! current user is not $ROBOT, use $SUDO"
fi

latestDigest() { # _key_digest="$1"
   $SUDO ssh $ssh_optoins "$SECHOST" "cat $PROJECTS_DIR/$project/$1"|awk '{print $1}'
}

fileDigest() { "$MD5" -a 256 "$1" 2>/dev/null | awk '{print $1}'; }

checkDigest() { # _local_key="$1" _remote_digest="$2"
   [ "$(fileDigest "$1")" = "$(latestDigest "$2")" ]; return $?
}

# Compares stdin with <original> file. Returns 0 for equal data, 1 otherwise.
# Usage: cat <file1> <file2> ... | checkSplit <original>
checkSplit() { diff -q "$1" <( cat ) &>/dev/null; }
checkSize() { (( $(stat -c %s $_new) == 48 )); }

getKey() {
   local _src="$1" _dst="${2:-"./"}"
   command rsync -c -e "$SUDO ssh $ssh_optoins" \
      "${SECHOST}:${PROJECTS_DIR}/${project}/${_src}" "$_dst" &> /dev/null
   return $?
}

getKeyFailsafe() {
   local _src="$1" _dst="${2:-"./"}" _i=0
   until getKey "$_src" "$_dst"; do
      ((++_i, _i>2))&&return 1; sleep ${sleep_on_error:-$((RANDOM%10))};
      log_action "getKey failed retry #$_i"
   done
   return $?
}

rotateKeys() {
   local _keyfile="$1"
   local _prev="${_keyfile}.prev" _prevprev="${_keyfile}.prevprev"

   # Write access to keyfile dir is required to move files
   if [ -w "`pwd`" ]; then
      if ${rotate_keys:-false}; then
         mv -f "$_prev"    "$_prevprev" 2>/dev/null || true
         chmod 0440 $_prevprev |&:
         cp -f "$_keyfile" "$_prev"     2>/dev/null || true
         chmod 0440 $_prev |&:
         log_action "Keyfile '$_keyfile' rotation succeed."
      fi
      return 0
   else
      return 1
   fi
}

extractSessionKey() {
   local _keyfile="$1" _keynum="$2"

   awk "BEGIN{k=0; p=0}
   /-----BEGIN SESSION TICKET KEY-----/{k++; p=1}
   k == $_keynum && p{print}
   /-----END SESSION TICKET KEY-----/{p=0}" "$_keyfile"
}

splitKeys() {
   local _keyfile="${1}"
   local _key_1="${_keyfile}.1" _key_2="${_keyfile}.2" _key_3="${_keyfile}.3"

   if ${split_keys:-false}; then
      extractSessionKey "$_keyfile" 1 > "$_key_1"
      extractSessionKey "$_keyfile" 2 > "$_key_2"
      extractSessionKey "$_keyfile" 3 > "$_key_3"
      if cat "$_key_1" "$_key_2" "$_key_3"|checkSplit "$_keyfile"; then
         log_action "Keyfile '$_keyfile' has been successfully splitted."
         return 0
      else
         log_action "WARNING! Can't split keyfile '$_keyfile'." \
            "Check keyfile directory permissions."
         return 1
      fi
   fi
   return 0
}

updateKey() {
   local _new="$1" _current="$2"

   if checkSize $_new ;then
      rotateKeys "$_current"||{ log_action "ERROR! rotateKeys failed";return 1;}
      mv "$_new" "$_current"
      chmod 0440 $_current |&:
      splitKeys  "$_current"||{ log_action "ERROR! splitKeys failed"; return 1;}
   else
      log_action "ERROR! key size not equal 48bytes!"
      return 1
   fi
}

service_reloadKeys() {
   if [ -n "$(command -v "$service_reloadAction")" ]; then
      "$service_reloadAction" "${local_key_dir}/${local_key_file}"
      return 0
   else
      log_action "ERROR! Unknown key reload script: '$service_reloadAction'"
      return 1
   fi
}

on_update_success() {
   # Here you can specify actions, performed after successful key update
   # (update succeed && digest check succeed)
   if service_reloadKeys; then
      return 0
   else
      log_action "ERROR! Service failed to reload key."
      return 1
   fi
}

on_update_fail() {
   # Here you can specify actions, performed after key update failure
   # (update succeed && digest check failed)
   return 0
}

reload_nothing() { return 0; }

reload_nginx() {    # Make nginx reload configuration to reload keys
   log_action "Making nginx to reload keys..."
   command service nginx reload
}

reload_balancer() {    # Make balancer reload ssl tickets
   log_action "Making balancer to reload keys..."
   return 0
   #curl ...
}

if ! touch "${logfile}"; then   # Check logfile is writable
   log_action "WARNING! Logfile is not writable"
fi

project="$1"
remote_key_file="${2%${KEY_SUFFIX}}$KEY_SUFFIX" # ensure suffix
remote_key_digest="${remote_key_file}$DIGEST_SUFFIX"
local_key_dir="$3"
local_key_file="${remote_key_file##*/}"
# dirty hack; why?
cd "$local_key_dir"||{ log_action "ERROR! Can't cd in target directory";mkdir $local_key_dir;exit 1; }

# Check availability of ${SECHOST}
$(command -v nc) -z $SECHOST 22 &>/dev/null
if [ "$?" -ne "0" ]; then
   log_action "WARNING! $SECHOST is unreachable."
   echo "WARNING! $SECHOST is unreachable." > ${TMPDIR}/${local_key_file}.status
   exit 1
fi

if ${force:-false} ; then
   # Forced update. Get key, check, rotate, update, split.
   if getKeyFailsafe "$remote_key_file" "temp.key" \
         && checkDigest "temp.key" $remote_key_digest &>/dev/null \
         && updateKey "temp.key" "$local_key_file"
      then
      rm -f ${TMPDIR}/${local_key_file}.trigger
      on_update_success
      log_action "Forced key update succeed."
      exit 0
   else
      log_action "WARNING! Forced key update failed."
      on_update_fail
      exit 1
   fi
else
   # No 'forced' update. Check key first, update only on hashsum mismatch.
   if checkDigest "$local_key_file" "$remote_key_digest" &>/dev/null; then
      log_action "Key has been checked successfully. No update required."
      rm -f ${TMPDIR}/${local_key_file}.trigger
      exit 0
   else
      printf "\n" >> "${TMPDIR}/${local_key_file}.trigger"
      run_times="$(wc -l < "${TMPDIR}/${local_key_file}.trigger")"

      if getKeyFailsafe "$remote_key_file" "temp.key" \
         && checkDigest "temp.key" "$remote_key_digest" &>/dev/null \
         && updateKey "temp.key" "$local_key_file"; then
      rm -f ${TMPDIR}/${local_key_file}.trigger
      on_update_success
      log_action "Local key copy was not equal to remote one." \
                 "Key has been successfully updated."
      else
         log_action "WARNING! Local copy update failed! Local file differs" \
                    "from remote one after $run_times script runs."
         on_update_fail
         exit 1
      fi
   fi
fi
