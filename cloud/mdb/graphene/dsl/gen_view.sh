#!/bin/bash

cat > gen_view.groovy <<EOF
listView('Graphene') {
    description('All Graphene Jobs')
    columns {
        status()
        weather()
        name()
        lastSuccess()
        lastFailure()
        lastDuration()
        buildButton()
    }
    jobs {
EOF

while read -r i
do
    echo "        name(\"$i\")" >> gen_view.groovy
done < <(cat -- *.groovy | grep 'job("' | cut -d\" -f2)

cat >> gen_view.groovy <<EOF
    }
}
EOF
