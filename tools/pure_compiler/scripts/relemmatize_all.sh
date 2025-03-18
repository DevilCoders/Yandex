#!/usr/bin/env sh

for lang in rus arm blr bul cze fre ger ita kaz pol por rum spa tat tur ukr eng; do
sh relemmatize_pure.sh pure.lg.groupedtrie.$lang pure.lg.groupedtrie.$lang
done
