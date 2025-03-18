package utils // GenCert will create and populate a certificate file and a key file
import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"fmt"
	"math/big"
	"net"
	"os"
	"os/user"
	"path"
	"time"
)

/*
 * Generate a list of names for which the certificate will be valid.
 * This will include the hostname and ip address
 */
func mynames() ([]string, error) {
	h, err := os.Hostname()
	if err != nil {
		return nil, err
	}

	ret := []string{h, "127.0.0.1/8", "::1/128"}
	return ret, nil
}

// GenerateMemCert creates client or server certificate and key pair,
// returning them as byte arrays in memory.
func GenerateMemCert(client bool, addHosts bool) ([]byte, []byte, error) {
	privk, err := ecdsa.GenerateKey(elliptic.P384(), rand.Reader)
	if err != nil {
		return nil, nil, fmt.Errorf("Failed to generate key: %v", err)
	}

	validFrom := time.Now()
	validTo := validFrom.Add(10 * 365 * 24 * time.Hour)

	serialNumberLimit := new(big.Int).Lsh(big.NewInt(1), 128)
	serialNumber, err := rand.Int(rand.Reader, serialNumberLimit)
	if err != nil {
		return nil, nil, fmt.Errorf("Failed to generate serial number: %v", err)
	}

	userEntry, err := user.Current()
	var username string
	if err == nil {
		username = userEntry.Username
		if username == "" {
			username = "UNKNOWN"
		}
	} else {
		username = "UNKNOWN"
	}

	hostname, err := os.Hostname()
	if err != nil {
		hostname = "UNKNOWN"
	}

	template := x509.Certificate{
		SerialNumber: serialNumber,
		Subject: pkix.Name{
			Organization: []string{"yandex.net"},
			CommonName:   fmt.Sprintf("%s@%s", username, hostname),
		},
		NotBefore: validFrom,
		NotAfter:  validTo,

		KeyUsage:              x509.KeyUsageKeyEncipherment | x509.KeyUsageDigitalSignature,
		BasicConstraintsValid: true,
	}

	if client {
		template.ExtKeyUsage = []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth}
	} else {
		template.ExtKeyUsage = []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth}
	}

	if addHosts {
		hosts, err := mynames()
		if err != nil {
			return nil, nil, fmt.Errorf("Failed to get my hostname: %v", err)
		}

		for _, h := range hosts {
			if ip, _, err := net.ParseCIDR(h); err == nil {
				if !ip.IsLinkLocalUnicast() && !ip.IsLinkLocalMulticast() {
					template.IPAddresses = append(template.IPAddresses, ip)
				}
			} else {
				template.DNSNames = append(template.DNSNames, h)
			}
		}
	} else if !client {
		template.DNSNames = []string{"unspecified"}
	}

	derBytes, err := x509.CreateCertificate(rand.Reader, &template, &template, &privk.PublicKey, privk)
	if err != nil {
		return nil, nil, fmt.Errorf("Failed to create certificate: %v", err)
	}

	data, err := x509.MarshalECPrivateKey(privk)
	if err != nil {
		return nil, nil, err
	}

	cert := pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: derBytes})
	key := pem.EncodeToMemory(&pem.Block{Type: "EC PRIVATE KEY", Bytes: data})

	return cert, key, nil
}

func GenCert(certf string, keyf string, certtype bool, addHosts bool) error {
	/* Create the basenames if needed */
	dir := path.Dir(certf)
	err := os.MkdirAll(dir, 0750)
	if err != nil {
		return err
	}
	dir = path.Dir(keyf)
	err = os.MkdirAll(dir, 0750)
	if err != nil {
		return err
	}

	certBytes, keyBytes, err := GenerateMemCert(certtype, addHosts)
	if err != nil {
		return err
	}

	certOut, err := os.Create(certf)
	if err != nil {
		return fmt.Errorf("Failed to open %s for writing: %v", certf, err)
	}
	certOut.Write(certBytes)
	certOut.Close()

	keyOut, err := os.OpenFile(keyf, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0600)
	if err != nil {
		return fmt.Errorf("Failed to open %s for writing: %v", keyf, err)
	}
	keyOut.Write(keyBytes)
	keyOut.Close()
	return nil
}
