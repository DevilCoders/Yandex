HOST=`hostname -f`
PORT="8088"
CHECK=`curl --silent --max-time 2 "http://$HOST:$PORT/hosts_info"`
if [[ "$?" -eq "0" ]]; then
        if [[ `echo $CHECK | awk '{print $NF}'` == "opened" ]]; then
                echo "0;Ok"
        else
                echo "2;$CHECK"
        fi
else
        echo "2;$HOST:$PORT not available!"
fi
