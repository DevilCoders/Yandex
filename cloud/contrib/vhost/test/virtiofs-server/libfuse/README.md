# Libfuse

This is patched version of **libfuse** (~3.8) that was imported to **qemu** as
part of **virtiofsd**, stripped of all unused parts and patched to read fuse
requests from virtio. From **qemu** it was imported to arcadia
(*arcadia/contrib/virtiofsd*) and stripped of **qemu** dependencies, and finally
it was imported here and stripped of arcadia stuff (also fuse.h was remaned to
fuse_kernel.h to avoid confusion with library interface).

It is incomplete and provides just part of functionality so it's built to static
library, to become useful it requires definitions of functions declared in
*fuse_virtio.h* to link with.
