#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc79() {
    // yahoo with https (url by gotmanov@).
    {
        TInfo info(SE_YAHOO, ST_WEB, "3+ì", SF_SEARCH);

        KS_TEST_URL("https://it.search.yahoo.com/search;_ylt=A9mSs2Ichq5T3isA9LkaDQx.;_ylc=X1MDMjExNDcxOT\
AwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10YgRncHJpZAMEbXRlc3RpZA\
NVSTAxJTNEVklQSVQwMQRuX3JzbHQDMARuX3N1Z2cDMARvcmlnaW4DaXQuc2VhcmNoLnlhaG9vLmNvbQRwb3MDBHBxc3RyAw\
RwcXN0cmwDMARxc3RybAMwBHF1ZXJ5AwR0X3N0bXADMTQwMzk0NzQ2NDY5MQR2dGVzdGlkA1ZJUElUMDE-;_ylc=X1MDMjEx\
NDcxOTAwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10YgRncHJpZAMEbXRl\
c3RpZANVSTAxJTNEVklQSVQwMQRuX3JzbHQDMARuX3N1Z2cDMARvcmlnaW4DaXQuc2VhcmNoLnlhaG9vLmNvbQRwb3MDMARw\
cXN0cgMEcHFzdHJsAwRxc3RybAMzBHF1ZXJ5AzMrNgR0X3N0bXADMTQwMzk0NzQ2NTg5NgR2dGVzdGlkA1ZJUElUMDE-;_yl\
c=X1MDMjExNDcxOTAwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10YgRncH\
JpZAMEbXRlc3RpZANVSTAxJTNEVklQSVQwMQRuX3JzbHQDMARuX3N1Z2cDMARvcmlnaW4DaXQuc2VhcmNoLnlhaG9vLmNvbQ\
Rwb3MDMARwcXN0cgMEcHFzdHJsAwRxc3RybAMzBHF1ZXJ5AzMrNgR0X3N0bXADMTQwMzk0NzQ3NDk2MAR2dGVzdGlkA1ZJUE\
lUMDE-;_ylc=X1MDMjExNDcxOTAwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3Vy\
eS10YgRncHJpZAMEbXRlc3RpZANVSTAxJTNEVklQSVQwMQRuX3JzbHQDMARuX3N1Z2cDMARvcmlnaW4DaXQuc2VhcmNoLnlh\
aG9vLmNvbQRwb3MDMARwcXN0cgMEcHFzdHJsAwRxc3RybAMzBHF1ZXJ5AzMrNgR0X3N0bXADMTQwMzk0NzQ3NDk4OAR2dGVz\
dGlkA1ZJUElUMDE-;_ylc=X1MDMjExNDcxOTAwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZn\
IDbGlua3VyeS10YgRncHJpZAMEbXRlc3RpZANVSTAxJTNEVklQSVQwMQRuX3JzbHQDMARuX3N1Z2cDMARvcmlnaW4DaXQuc2\
VhcmNoLnlhaG9vLmNvbQRwb3MDMARwcXN0cgMEcHFzdHJsAwRxc3RybAMzBHF1ZXJ5AzMrNgR0X3N0bXADMTQwMzk0NzQ3NT\
AzNAR2dGVzdGlkA1ZJUElUMDE-;_ylc=X1MDMjExNDcxOTAwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZz\
JTNEMjkEZnIDbGlua3VyeS10YgRncHJpZAMEbXRlc3RpZANVSTAxJTNEVklQSVQwMQRuX3JzbHQDMARuX3N1Z2cDMARvcmln\
aW4DaXQuc2VhcmNoLnlhaG9vLmNvbQRwb3MDMARwcXN0cgMEcHFzdHJsAwRxc3RybAMzBHF1ZXJ5AzMrNgR0X3N0bXADMTQw\
Mzk0NzQ3NTA1NQR2dGVzdGlkA1ZJUElUMDE-;_ylc=X1MDMjExNDcxOTAwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2Yi\
UzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10YgRncHJpZAMEbXRlc3RpZANVSTAxJTNEVklQSVQwMQRuX3JzbHQDMARuX3N1Z2\
cDMARvcmlnaW4DaXQuc2VhcmNoLnlhaG9vLmNvbQRwb3MDMARwcXN0cgMEcHFzdHJsAwRxc3RybAMzBHF1ZXJ5AzMrNgR0X3\
N0bXADMTQwMzk0NzQ3NTA4NwR2dGVzdGlkA1ZJUElUMDE-;_ylc=X1MDMjExNDcxOTAwMgRfcgMyBGJjawM3dHAwdTN0OWZo\
ODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10YgRncHJpZAMEbXRlc3RpZANVSTAxJTNEVklQSVQwMQRuX3JzbHQD\
MARuX3N1Z2cDMARvcmlnaW4DaXQuc2VhcmNoLnlhaG9vLmNvbQRwb3MDMARwcXN0cgMEcHFzdHJsAwRxc3RybAMzBHF1ZXJ5\
AzMrNgR0X3N0bXADMTQwMzk0NzQ3NTExOQR2dGVzdGlkA1ZJUElUMDE-;_ylc=X1MDMjExNDcxOTAwMgRfcgMyBGJjawM3dH\
AwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10YgRncHJpZAMEbXRlc3RpZANVSTAxJTNEVklQSVQwMQ\
RuX3JzbHQDMARuX3N1Z2cDMARvcmlnaW4DaXQuc2VhcmNoLnlhaG9vLmNvbQRwb3MDMARwcXN0cgMEcHFzdHJsAwRxc3RybA\
MzBHF1ZXJ5AzMrNgR0X3N0bXADMTQwMzk0NzQ3NTE1MgR2dGVzdGlkA1ZJUElUMDE-;_ylc=X1MDMjExNDcxOTAwMgRfcgMy\
BGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10YgRncHJpZAMEbXRlc3RpZANVSTAxJTNE\
VklQSVQwMQRuX3JzbHQDMARuX3N1Z2cDMARvcmlnaW4DaXQuc2VhcmNoLnlhaG9vLmNvbQRwb3MDMARwcXN0cgMEcHFzdHJs\
AwRxc3RybAMzBHF1ZXJ5AzMrNgR0X3N0bXADMTQwMzk0NzQ3NTE4NgR2dGVzdGlkA1ZJUElUMDE-;_ylc=X1MDMjExNDcxOT\
AwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10YgRncHJpZAMEbXRlc3RpZA\
NVSTAxJTNEVklQSVQwMQRuX3JzbHQDMARuX3N1Z2cDMARvcmlnaW4DaXQuc2VhcmNoLnlhaG9vLmNvbQRwb3MDMARwcXN0cg\
MEcHFzdHJsAwRxc3RybAMzBHF1ZXJ5AzMrNgR0X3N0bXADMTQwMzk0NzQ3NTIyMwR2dGVzdGlkA1ZJUElUMDE-;_ylc=X1MD\
MjExNDcxOTAwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10YgRncHJpZAME\
bXRlc3RpZANVSTAxJTNEVklQSVQwMQRuX3JzbHQDMARuX3N1Z2cDMARvcmlnaW4DaXQuc2VhcmNoLnlhaG9vLmNvbQRwb3MD\
MARwcXN0cgMEcHFzdHJsAwRxc3RybAMzBHF1ZXJ5AzMrNgR0X3N0bXADMTQwMzk0NzQ3NTI1MwR2dGVzdGlkA1ZJUElUMDE-\
;_ylc=X1MDMjExNDcxOTAwMgRfcgMyBGJjawM3dHAwdTN0OWZoODMxJTI2YiUzRDMlMjZzJTNEMjkEZnIDbGlua3VyeS10Yg\
RncHJpZANNRzVzejQyblJoU0wubkVJSU5xR1ZBBG10ZXN0aWQDVUkwMSUzRFZJUElUMDEEbl9yc2x0AzAEbl9zdWdnAzcEb3\
JpZ2luA2l0LnNlYXJjaC55YWhvby5jb20EcG9zAzAEcHFzdHIDBHBxc3RybAMEcXN0cmwDMwRxdWVyeQMzK.wEdF9zdG1wAz\
E0MDM5NDc0OTc4NDEEdnRlc3RpZANWSVBJVDAx?gprid=MG5sz42nRhSL.nEIINqGVA&pvid=vwh5MDIxMi5.5B4fUvigYQT\
GMTg4LlOuhhz_wd0o&p=3%2Bì&fr=linkury-tb&fr2=sa-gp&type=hp1000&iscqry=",
                    info);
    }
}
