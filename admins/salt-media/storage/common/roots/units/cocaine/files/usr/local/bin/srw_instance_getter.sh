#!/bin/bash

2>/dev/null

if [ $1 = "qloud" ]
then
  DC=$(hostname -f | egrep -o "^[a-z]{3}");
else
  DC=$2;
fi

ISOLATE_CONF="/etc/cocaine-isolate-daemon/cocaine-isolate-daemon.conf"
META_NAME="$(jq -r '.["isolate"]["porto"]["args"]["meta_name"]' $ISOLATE_CONF 2>/dev/null)"
META_PATH=""
if [[ $META_NAME != "" ]] || [[ $META_NAME != "null" ]]; then
  META_PATH=$(/usr/sbin/portoctl list -1 | egrep "$META_NAME$" | head -n1)
fi

for i in $(/usr/sbin/portoctl list -1 | egrep "_.{8}-.{4}-.{4}-.{4}-.{12}$"); do
  echo `/usr/sbin/portoctl get $i absolute_name`" "`/usr/sbin/portoctl get $i hostname`" "`/usr/sbin/portoctl get $i env | tr "\t"," " "_"`;
done | gawk -v dc=$DC -v env=$1 '{
  n=split($1,nf,"/");
  base_cname=nf[n];
  norm_appver=substr(base_cname,0,length(base_cname)-37);
  appver_sep_idx=index(base_cname,"_");
  match(substr(base_cname,0,appver_sep_idx-1), "^[\\.a-zA-Z0-9\\-]{,110}", napp);
  app=napp[0];
  match(substr(norm_appver,appver_sep_idx+1,length(norm_appver)-1), "^[\\.a-zA-Z0-9\\-]{,110}", nver);
  ver=nver[0];
  if (length(ver)==0) { ver="0";};
  print("portocontainer="$1" a_itype_isolatesrw a_geo_"dc" a_ctype_"env" a_prj_"app" a_tier_"ver);
  container_fqdn=$2;
  if (length($3) > 2) {
    ne=split($3, envs, ";");
    gotcha=0;
    for (i=1; i <= ne; i++) {
      ef=split(envs[i], e, "=");
      if (ef < 2) {continue;};
      switch (e[1]) {
        case "UNISTATINFO":
          ni=split(e[2], uinfo, "/");
          if (ni > 1) {
            itype=uinfo[2]
          } else {
            itype="unknown";
          };
          port=uinfo[1];
          gotcha=1;
          break;
        case "monitoringYasmagentEndpoint":
          if (e[2] ~ /^http:.*$/) {
            monitoringYasmagentEndpoint=" monitoringYasmagentEndpoint="e[2];
          } else {
            monitoringYasmagentEndpoint=" monitoringYasmagentEndpoint=http://"container_fqdn""e[2];
          };
          gotcha=1;
          break;
        default:
          break;
      };
    };
    if (gotcha) {
      print(container_fqdn":"port"@"app" a_itype_"itype" a_geo_"dc" a_ctype_"env" a_prj_"app" a_tier_"ver" yasmUnistatFqdn="container_fqdn);
    };
  };
}'

if [[ $META_PATH != "" ]]; then
  echo "portocontainer=$(/usr/sbin/portoctl get $META_PATH absolute_name) a_itype_isolatesrw a_geo_$DC a_ctype_$1 a_prj_allapps a_tier_cocaine"
fi

