# hxroot
More modern re-implementation of fakechroot+fakeroot. Supports glibc for termux (use with `grun --shell`). Automatically falls back to proot for unsupported binaries

Same as proot, please don't use this to sandbox untrusted applications. Fakechroot is extremely leaky, you only need `unset LD_PRELOAD` to escape

# License
SPDX-License-Identifier: Apache-2.0
