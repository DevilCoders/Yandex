# Starts e2e build on the top of queque, waits for finish and print status
# using |sh ./deploy_and_e2e.sh mqtt|

set -e
E2E_BUILD_CONFIG=Cloud_Api_E2ePublicApiTest_E2eIotPreprod


WRONG_TS_TOKEN_MSG='set TEAMCITY_TOKEN from https://oauth.yandex-team.ru/authorize?response_type=token&client_id=7feab4f3dd964c95b57ba7b6fa085cc5'
if [ "${TEAMCITY_TOKEN:-}" == "" ]
then
  echo "${WRONG_TS_TOKEN_MSG}"
  exit 1
fi

CHECK_CFG=$(curl -s -H "Authorization: OAuth ${TEAMCITY_TOKEN}"   -H "Accept: application/json" \
  "http://teamcity.yandex-team.ru/app/rest/builds/?locator=buildType:${E2E_BUILD_CONFIG}")

CHECK_CFG_COUNT=$(echo "$CHECK_CFG" |  jq -r ".count" | perl -e'print join("", <>); # swallow error')
if [ "${CHECK_CFG_COUNT:-0}" == "0" ]
then
  echo "ACHTUNG:"
  echo "${CHECK_CFG}"
  echo
  echo "Maybe ${WRONG_TS_TOKEN_MSG}"
  exit 1
fi


sh ./deploy.sh "$@"


BUILD_SPEC=$(curl -s -H "Authorization: OAuth ${TEAMCITY_TOKEN}" -H "Content-Type: application/xml" -H "Accept: application/json" -X POST  \
  "https://teamcity.yandex-team.ru/app/rest/buildQueue" --data "
<build personal=\"false\" >
   <triggeringOptions queueAtTop=\"true\"/>
   <buildType id=\"${E2E_BUILD_CONFIG}\"/>
   <comment><text>Deploy check</text></comment>
   <tags>
       <tag name=\"iot_deploy_check\"/>
   </tags>
 </build>
")

BUILD_ID=$(echo "${BUILD_SPEC:?}" | jq ".id")
echo "start build ${BUILD_ID}"

while :
do
RESULT="$(curl -s -H "Authorization: OAuth ${TEAMCITY_TOKEN}"   -H "Accept: application/json" \
  "https://teamcity.yandex-team.ru/app/rest/builds/${BUILD_ID}" )"

STATE=$(echo "$RESULT" | jq -r '.state')
STATUS=$(echo "$RESULT" | jq -r '.status')
COMPL=$(echo "$RESULT" | jq -r '.percentageComplete')

echo "$COMPL\t$STATUS\t$STATE"

[ "${STATE:?}" == 'finished' ] && break
sleep 3
done

echo "Build: https://teamcity.yandex-team.ru/buildConfiguration/${E2E_BUILD_CONFIG}/${BUILD_ID}"
echo "====> ${STATE} $STATUS"

if [ "${STATUS}" != "SUCCESS" ]; then exit 1; fi
