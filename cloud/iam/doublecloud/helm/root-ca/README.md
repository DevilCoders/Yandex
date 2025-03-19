## Certificates

### Root CA certs
```
# doublecloud-aws-prod
SUBJECT="/C=DE/ST=Hesse/L=Frankfurt/O=DobleCloud/CN=prod.iam.internal.double.tech"
YAV_SECRET_ID=sec-01fgp4w6w14jxxtzsymwxd6rht
# doublecloud-aws-preprod
SUBJECT="/C=DE/ST=Hesse/L=Frankfurt/O=DataCloud/CN=preprod.iam.internal.yadc.tech"
YAV_SECRET_ID=sec-01f808yngmqs5cwx8ymqzs7vtm
# doublecloud-aws-preprod-legacy
SUBJECT="/C=DE/ST=Hesse/L=Frankfurt/O=DataCloud/CN=internal.yadc.io"
YAV_SECRET_ID=sec-01f808yngmqs5cwx8ymqzs7vtm (XXX use CA_iam_cert_legacy.key and CA_iam_cert_legacy.crt for preprod-legacy)
# doublecloud-yc-preprod
SUBJECT="/C=RU/ST=Russian Federation/L=Moscow/O=DataCloud/CN=yandex.datacloud.net"
YAV_SECRET_ID=sec-01f808m2tmk9avwp0w2axjgamr
```

```
openssl genrsa -out CA_iam_cert.key 4096
openssl req -x509 -new -days 3660 -subj "${SUBJECT?}" -key CA_iam_cert.key -out CA_iam_cert.crt
# yav create version ${YAV_SECRET_ID?} -u -f CA_iam_cert.crt=CA_iam_cert.crt CA_iam_cert.key=CA_iam_cert.key > /dev/null
rm -f CA_iam_cert.key
rm -f CA_iam_cert.crt
```

### Store into helm secrets
Use crt and key files content for `../datacloud-common/${ENV}/${CSP}/common/secrets.yaml`
with keys `certmanager:ca:tls.crt` and `certmanager:ca:tls.key`


### Usage
For developer hosts we need to install RootCA.
You can download cert from yav, place it into directory `/usr/local/share/ca-certificates/` on you machine and run `update-ca-certificates`
```
sudo cp .../ca-datacloud-preprod.crt /usr/local/share/ca-certificates/ca-datacloud-preprod.crt
sudo update-ca-certificates
```


### DO NOT NEEDED ~~Prepare Java Truststore~~
```
echo "<content of ../datacloud-common/${ENV}/${CSP}/common/secrets:common:truststoreBase64Enc>" | base64 -d > truststore.file
```
List entries
```
keytool -keystore truststore.file -list
```

Export certificate
```
keytool -keystore truststore.file -exportcert -alias <ALIAS(from list)> -rfc -file <OUT_FILENAME>
```

Import certificate
```
keytool -keystore truststore.file -importcert -alias <ALIAS> -file <IN_FILENAME>
```

--Delete certificate--
```
keytool -keystore truststore.file -delete -alias <ALIAS>
```

Save truststore into secrets
```
cat truststore.file | base64 --wrap=0  ----> store it into: 
helm secrets edit ../datacloud-common/${ENV}/${CSP}/common/secrets.yaml
```

### The OLD WAY. ~~Generate Intermediate Certificate~~
(Do not use for now)

1. Copy Root CA cert and key files
```
cp .../ca.key.rsa .
cp .../ca.crt .
```
2. Prepare the directory
```
touch index.txt
echo 1000 > serial
echo 1000 > crlnumber
```
3. Create the intermediate key
```
openssl genrsa -out intermediate.key.rsa 4096
```
4. Create the intermediate certificate request (CSR)
```
# doublecloud-aws
SUBJECT="/C=RU/ST=Russian Federation/L=Moscow/O=DataCloud/CN=DataCloud IAM Intermediate CA"
# doublecloud-yc
SUBJECT="/C=RU/ST=Russian Federation/L=Moscow/O=DataCloud/CN=Yandex DataCloud IAM Intermediate CA"

openssl req -new -config openssl.cnf -subj ${SUBJECT?} -key intermediate.key.rsa -out intermediate.csr
```
5. Sign intermediate CSR
```
openssl ca -config openssl.cnf -extensions v3_intermediate_ca -days 1830 -notext -batch -in intermediate.csr -out intermediate.crt
```
6. Delete  Root CA private key
```
rm ca.key.rsa
```
