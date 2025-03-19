#!/bin/bash
echo > debian/links
for i in `ls src/etc/xscript-multiple/conf-available/`; do echo -e "src/etc/xscript-multiple/conf-available/$i src/etc/xscript-multiple/conf-enabled/$i" >> debian/links; done
