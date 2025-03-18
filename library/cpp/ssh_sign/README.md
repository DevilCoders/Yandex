# Ssh sign
Library for signing data with private ssh keys
Analog of `library/python/ssh_sign`

## Customization

You can change priority/use other policies by creating analog of `SignByKey`/`SignByAny`.
See `sign.cpp` for details.

If you need new policy see `policies.{h,cpp}`

## Dsa

Dsa keys are not supported because `library/cpp/ssh` does not support them.

## PSS

PSS is not supported. You need to change TCustomRsaKeyPolicy (or create analog of it) if needed.
