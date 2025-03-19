#!/bin/bash

NUM_OK=5
LOGFILE=/var/log/rcmnd-mongo-backup-monitor.log

if [ -s /etc/monitoring/mongo-backup.conf ]; then
   source /etc/monitoring/mongo-backup.conf
fi


rm_rf(){
   rm -rfv "$@" &>> $LOGFILE
}

rotate_logs(){
   local logzise=$(stat -c "%s" $LOGFILE) limit=$(( (1024**2)*256 )) # 256MB
   if (( $logzise > $limit ));then
      echo "$(date '+%F_%T'): Rotate log" >> $LOGFILE;
      pigz -f $LOGFILE
   fi
}

rotate_and_monrun_backups() {
   echo "$(date '+%F_%T'): Monitoring runned!" >> $LOGFILE;

   local count _cmd_to_run=rm_rf first_arg="$1"
   if [[ "$first_arg" =~ ^(-d|--dry-run|-t|--tes) ]];then
      _cmd_to_run=echo;
   fi

   for backup in /opt/storage/mongo/rcmnd/rcmnd*; do
      if test -d $backup; then
         # Dangerous section
         echo "$(date '+%F_%T'): Start delete old backups for $backup" >> $LOGFILE;
         find $backup -maxdepth 1 ! -path $backup -type d -mtime +30 -exec bash -c "LOGFILE=$LOGFILE ${_cmd_to_run}"' $0 "$@"' {} +
         echo "$(date '+%F_%T'): End delete old backups for $backup" >> $LOGFILE;
         # END Dangerous section

         # monrun section
         count=`find $backup -iname '*dump*' -mtime -$((NUM_OK+1)) 2>/dev/null | wc -l`;

         if ((count < NUM_OK)); then
            num_lost=$((NUM_OK - count))
            if [ -z "${error[num_lost]}" ]
               then error[num_lost]="Lost $num_lost backups: "
               else error[num_lost]+=','
            fi
            error[num_lost]+=${backup##*/}
         fi
         # END monrun section
      fi
   done

   IFS=';'
   echo "$((${error[*]:+2}+0));${error[*]:-Ok}";
}

export -f rm_rf
rotate_and_monrun_backups $1

rotate_logs  &>/dev/null &
