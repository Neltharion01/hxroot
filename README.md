# hxroot
More modern re-implementation of fakechroot+fakeroot. Supports glibc for termux (use with `grun --shell`). Automatically falls back to proot for unsupported binaries

Same as proot, please don't use this to sandbox untrusted applications. Fakechroot is extremely leaky, you only need `unset LD_PRELOAD` to escape

# Usage
First, obtain rootfs somewhere. I recommend using proot-distro to download, then grab it at `$PRETIX/var/lib/proot-distro`. Then convert all absolute symlinks with the `hxconvert` script

It is required because hxroot doesn't translate symlinks on the fly (for simplity and performance reasons). If you wish to revert the change, run `hxunconvert`

# License
SPDX-License-Identifier: Apache-2.0
