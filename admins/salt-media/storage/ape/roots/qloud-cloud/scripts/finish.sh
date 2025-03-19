#!/bin/bash
find /etc/sv -type f -name 'run' | egrep -v "log|yasmagent" | sed 's/run$/finish/g' | while read name
do
    cat > $name <<EOF
#!/bin/bash
echo [\`date\`] \`pwd | cut -d/ -f4\` >> /tmp/runit_flap_log
EOF
done
find /etc/sv -type f -name 'finish' -exec chmod +x {} +

