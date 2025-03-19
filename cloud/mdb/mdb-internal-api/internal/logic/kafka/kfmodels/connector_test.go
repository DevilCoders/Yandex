package kfmodels

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestKafkaConnector_ValidatePort(t *testing.T) {
	t.Run("PortValidate_Error_ZeroValue", func(t *testing.T) {
		port := "0"
		err := ValidatePort(port)
		require.Error(t, err)
		require.Equal(t, "fqdn's port \"0\" is not valid", err.Error())
	})

	t.Run("PortValidate_Success_NonZeroValue", func(t *testing.T) {
		port := "120"
		err := ValidatePort(port)
		require.NoError(t, err)
	})

	t.Run("PortValidate_Error_NotNumberValue", func(t *testing.T) {
		port := "120ad"
		err := ValidatePort(port)
		require.Error(t, err)
		require.Equal(t, "fqdn's port \"120ad\" is not valid", err.Error())
	})

	t.Run("PortValidate_Error_LeadingZeroesInPort", func(t *testing.T) {
		port := "0100"
		err := ValidatePort(port)
		require.Error(t, err)
		require.Equal(t, "fqdn's port \"0100\" is not valid", err.Error())
	})

	t.Run("PortValidate_Error_NumberNotInRange", func(t *testing.T) {
		port := "80001"
		err := ValidatePort(port)
		require.Error(t, err)
		require.Equal(t, "port "+port+" not in range [1, 65535]", err.Error())
	})
}

func TestKafkaConnector_ValidIpv4Address(t *testing.T) {
	t.Run("IpAddressValidate_Success_CorrectIpv4Address_Case1", func(t *testing.T) {
		ip := "120.120.50.50"
		result := IsValidIpv4Address(ip)
		require.True(t, result)
	})

	t.Run("IpAddressValidate_Success_CorrectIpv4Address_Case2", func(t *testing.T) {
		ip := "0.0.0.0"
		result := IsValidIpv4Address(ip)
		require.True(t, result)
	})

	t.Run("IpAddressValidate_Success_CorrectIpv4Address_Case3", func(t *testing.T) {
		ip := "255.255.255.255"
		result := IsValidIpv4Address(ip)
		require.True(t, result)
	})

	t.Run("IpAddressValidate_Success_CorrectIpv4Address_Case4", func(t *testing.T) {
		ip := "128.128.0.255"
		result := IsValidIpv4Address(ip)
		require.True(t, result)
	})

	t.Run("IpAddressValidate_Success_CorrectIpv4Address_Case5", func(t *testing.T) {
		ip := "5.5.5.5"
		result := IsValidIpv4Address(ip)
		require.True(t, result)
	})

	t.Run("IpAddressValidate_Invalid_NotValidMask_Case1", func(t *testing.T) {
		ip := "05.5.5.5"
		result := IsValidIpv4Address(ip)
		require.False(t, result)
	})

	t.Run("IpAddressValidate_Invalid_NotValidMask_Case2", func(t *testing.T) {
		ip := "5.5.c5.5"
		result := IsValidIpv4Address(ip)
		require.False(t, result)
	})

	t.Run("IpAddressValidate_Invalid_NotValidMask_Case3", func(t *testing.T) {
		ip := "5.5.011.5"
		result := IsValidIpv4Address(ip)
		require.False(t, result)
	})

	t.Run("IpAddressValidate_Invalid_NotValidMask_Case4", func(t *testing.T) {
		ip := "5.5.11.05"
		result := IsValidIpv4Address(ip)
		require.False(t, result)
	})

	t.Run("IpAddressValidate_Invalid_NotValidMask_Case5", func(t *testing.T) {
		ip := "abc"
		result := IsValidIpv4Address(ip)
		require.False(t, result)
	})

	t.Run("IpAddressValidate_Invalid_NotValidValueOfPart_Case1", func(t *testing.T) {
		ip := "0.1000.0.0"
		result := IsValidIpv4Address(ip)
		require.False(t, result)
	})

	t.Run("IpAddressValidate_Invalid_NotValidValueOfPart_Case2", func(t *testing.T) {
		ip := "300.1.1.10"
		result := IsValidIpv4Address(ip)
		require.False(t, result)
	})

	t.Run("IpAddressValidate_Invalid_NotValidValueOfPart_Case3", func(t *testing.T) {
		ip := "1.1.256.1"
		result := IsValidIpv4Address(ip)
		require.False(t, result)
	})

	t.Run("IpAddressValidate_Invalid_NotValidValueOfPart_Case4", func(t *testing.T) {
		ip := "1.1.1.256"
		result := IsValidIpv4Address(ip)
		require.False(t, result)
	})
}
