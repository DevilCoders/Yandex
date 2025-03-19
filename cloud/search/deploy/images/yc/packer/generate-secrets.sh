ENV="$1"

if [ "$ENV" == "prod" ]; then
	SA_JSON="ver-01ex4jknr886k8z37qnbtpbz46"
else
	SA_JSON="_"
fi

echo "${SA_JSON}"

cmd="ya vault get version"
if [ "x$YAV_LOGIN" != "x" ]; then
        cmd="$cmd --rsa-login $YAV_LOGIN"
fi

$cmd ${SA_JSON} -o sa.json > sa.json
