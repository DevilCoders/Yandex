# YCPasswordReset

YCPasswordReset is an PowerShell module which allows to reset users passwords via guest agent, running inside yandex cloud instance's.

## Installation

```
Install-Module -Name 'YCPasswordReset'
```

## Usage

```
Reset-YCUserPassword -Username Administrator -InstanceName MyWindowsInstance
```

## Description

Inside this module are several cmdlet's which helps to reset user password, but exported only one - Reset-YCUserPassword. You must install and configure `yc` before using this module, because it is a simple wrapper around it and does following:

* Create RSA key
* Create PasswordChangeRequest object with public key part
* Wrap request into Message object with required fields
* Update instance's metadata key
* Parse guest agent response from `COM4` serial port
* Decrypt password with private key
