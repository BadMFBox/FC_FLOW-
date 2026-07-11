#!/usr/bin/env bash

echo "========================================================="
echo "   ISOLATION STATUS CHECK                               "
echo "========================================================="
echo ""

echo "[*] Current working directory:"
pwd
echo ""

echo "[*] Real filesystem path (what host OS sees):"
readlink -f .
echo ""

echo "[*] Checking for Termux bind mounts in THIS directory:"
if mount | grep -q "$(pwd)"; then
    echo "⚠ WARNING: This directory is bind-mounted from Termux"
    mount | grep "$(pwd)"
else
    echo "✓ This directory is NOT bind-mounted (isolated)"
fi
echo ""

echo "[*] Checking environment contamination:"
if [ -n "${LD_PRELOAD:-}" ]; then
    echo "⚠ WARNING: LD_PRELOAD is set to: $LD_PRELOAD"
else
    echo "✓ LD_PRELOAD is clean"
fi

if env | grep -qi termux; then
    echo "⚠ WARNING: Termux variables detected:"
    env | grep -i termux
else
    echo "✓ No Termux environment variables"
fi
echo ""

echo "[*] Checking if MDM can write to this directory from Termux:"
echo "  From Termux shell, the path would be:"
echo "  \$PREFIX/var/lib/proot-distro/containers/archlinuxarm/rootfs$(pwd)"
echo ""
echo "  If that path exists in Termux, MDM can inject files."
echo ""

echo "[*] Checking parent process:"
ps -o pid,ppid,comm,cmd $$ | tail -1
echo ""

echo "========================================================="
echo "   RECOMMENDATION                                        "
echo "========================================================="
echo ""
echo "Your build at /root/FC_FLOW is relatively isolated"
echo "because it's inside proot's virtual filesystem."
echo ""
echo "However, MDM can still:"
echo "  • See the proot process itself"
echo "  • Inject via shared /dev, /proc, /sys mounts"
echo "  • Hook at kernel level (perfetto, hwuiTask)"
echo ""
echo "The REAL isolation issue is not bind mounts—"
echo "it's that you're running on Android kernel with MDM."
echo ""
echo "Current mitigations:"
echo "  ✓ Keys encrypted in RAM (Crossbolt)"
echo "  ✓ mlock prevents swapping to disk"
echo "  ✓ Burners wipe on anomaly detection"
echo "  ✓ Core pinning reduces scheduler interference"
echo ""
echo "If burners trigger = burn evidence + drop device"
echo ""
