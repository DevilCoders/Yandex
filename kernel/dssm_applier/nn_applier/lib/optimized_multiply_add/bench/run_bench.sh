./bench --format csv > bench_report.txt

echo "## last canonized run result" > README.md
cat bench_report.txt | sed 's/\t/ | /g' | awk -F "|" '
    {
        print "|" $0 "|";
        if(NR==1) {
            x = "";
            for(i = 0; i < NF; i+= 1) {
                x = x "| -----";
            }
            x = x "|"
            print x
        } 
    }
' >> README.md

echo "## run on cpu" >> README.md
echo '```' >> README.md
cat /proc/cpuinfo | grep "processor.*: 1$" -B 1000 | head -n -1 >> README.md
echo '```' >> README.md

echo "README.md updated"
