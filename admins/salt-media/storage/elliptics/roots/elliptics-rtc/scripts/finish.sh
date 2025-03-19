#!/bin/bash
find /etc/sv -type f -name 'run' | grep -v log | grep -v spacemimic | sed 's/run$/finish/g' | while read name
do
    cat > $name <<EOF
#!/bin/bash
echo [\`date\`] \`pwd | cut -d/ -f4\` >> /var/log/runit_flap.log
EOF
done
find /etc/sv -type f -name 'finish' -exec chmod +x {} +

