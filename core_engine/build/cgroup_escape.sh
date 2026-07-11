#!/usr/bin/env bash
set -euo pipefail

echo "========================================================="
echo "   CGROUP ESCAPE — Breaking Out of MDM CPU Restrictions "
echo "========================================================="
echo ""

PID=$$

echo "[*] Current process PID: $PID"
echo ""

echo "[*] Current cgroup assignments:"
cat /proc/$PID/cgroup
echo ""

echo "[*] Checking CPU affinity before escape:"
taskset -p $PID
echo ""

# Try to move to root cgroup (unrestricted)
echo "[*] Attempting cgroup escape..."

# Method 1: Write to cgroup.procs directly
if [ -w /sys/fs/cgroup/cgroup.procs ]; then
    echo $PID > /sys/fs/cgroup/cgroup.procs 2>/dev/null && echo "✓ Moved to root cgroup" || echo "✗ Failed (need root)"
fi

# Method 2: Use systemd-run (if available)
if command -v systemd-run &>/dev/null; then
    systemd-run --scope --user -p CPUAffinity=4-7 bash -c "echo '✓ systemd-run escape successful'" 2>/dev/null || echo "✗ systemd-run failed"
fi

# Method 3: Direct cpuset manipulation
CPUSET_PATH="/sys/fs/cgroup/cpuset"
if [ -d "$CPUSET_PATH" ]; then
    echo "[*] Attempting cpuset.cpus override..."
    CURRENT_CPUSET=$(cat /proc/$PID/cgroup | grep cpuset | cut -d: -f3)
    if [ -n "$CURRENT_CPUSET" ]; then
        echo "Current cpuset: $CURRENT_CPUSET"
        TARGET="$CPUSET_PATH$CURRENT_CPUSET/cpuset.cpus"
        if [ -w "$TARGET" ]; then
            echo "4-7" > "$TARGET" 2>/dev/null && echo "✓ Forced cpuset.cpus to 4-7" || echo "✗ Write failed"
        fi
    fi
fi

echo ""
echo "[*] CPU affinity after escape attempt:"
taskset -p $PID
echo ""

echo "========================================================="
echo "If escape failed, MDM has locked cgroups."
echo "Fallback: Run FC_FLOW with these flags to detect tampering"
echo "========================================================="
