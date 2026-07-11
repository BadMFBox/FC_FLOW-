#!/usr/bin/env bash
set -euo pipefail

CORE_TARGET=4

echo "========================================================="
echo "   FC_FLOW — CORE INVASION DIAGNOSTIC                   "
echo "========================================================="
echo ""
echo "[*] Checking what's running on Core $CORE_TARGET..."
echo ""

# Show all processes pinned to Core 4
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  PROCESSES CURRENTLY ON CORE $CORE_TARGET                          ║"
echo "╚═══════════════════════════════════════════════════════════╝"
ps -eLo pid,tid,psr,comm,cmd | awk -v core="$CORE_TARGET" '$3 == core {print}'
echo ""

# Show kernel threads
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  KERNEL THREADS (kworker, migration, ksoftirqd, etc)     ║"
echo "╚═══════════════════════════════════════════════════════════╝"
ps -eLo pid,tid,psr,comm | grep -E '\[.*\]' | awk -v core="$CORE_TARGET" '$3 == core {print}'
echo ""

# Show IRQ affinity (hardware interrupt handlers)
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  IRQ AFFINITY (which interrupts can fire on Core $CORE_TARGET)     ║"
echo "╚═══════════════════════════════════════════════════════════╝"
for irq in /proc/irq/*; do
    if [ -f "$irq/smp_affinity_list" ]; then
        affinity=$(cat "$irq/smp_affinity_list")
        irq_name=$(basename "$irq")
        # Check if Core 4 is in the affinity list
        if echo "$affinity" | grep -qE "(^|,)$CORE_TARGET(,|$)|^$CORE_TARGET-|$CORE_TARGET$"; then
            echo "IRQ $irq_name: $affinity"
        fi
    fi
done
echo ""

# Show CPU frequency governor (could cause scheduling jitter)
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  CPU FREQUENCY GOVERNOR (Core $CORE_TARGET)                        ║"
echo "╚═══════════════════════════════════════════════════════════╝"
if [ -f /sys/devices/system/cpu/cpu$CORE_TARGET/cpufreq/scaling_governor ]; then
    governor=$(cat /sys/devices/system/cpu/cpu$CORE_TARGET/cpufreq/scaling_governor)
    cur_freq=$(cat /sys/devices/system/cpu/cpu$CORE_TARGET/cpufreq/scaling_cur_freq)
    max_freq=$(cat /sys/devices/system/cpu/cpu$CORE_TARGET/cpufreq/scaling_max_freq)
    echo "Governor: $governor"
    echo "Current Freq: $cur_freq kHz"
    echo "Max Freq: $max_freq kHz"
else
    echo "cpufreq not available (might be running in container)"
fi
echo ""

# Show RCU callbacks (real-time killer)
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  RCU CALLBACKS (can cause involuntary context switches)  ║"
echo "╚═══════════════════════════════════════════════════════════╝"
if [ -f /sys/kernel/debug/rcu/rcu_preempt/rcudata ]; then
    echo "RCU data available:"
    grep -A 3 "cpu=$CORE_TARGET" /sys/kernel/debug/rcu/rcu_preempt/rcudata || echo "No RCU data for Core $CORE_TARGET"
else
    echo "RCU debug not available (need debugfs mounted or root)"
fi
echo ""

# Show scheduler stats
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  SCHEDULER STATS (Core $CORE_TARGET)                               ║"
echo "╚═══════════════════════════════════════════════════════════╝"
if [ -f /proc/schedstat ]; then
    echo "Schedstat available:"
    sed -n "$((CORE_TARGET + 2))p" /proc/schedstat
else
    echo "Schedstat not available"
fi
echo ""

echo "========================================================="
echo "   RECOMMENDATIONS                                       "
echo "========================================================="
echo ""
echo "If you see:"
echo "  • kworker threads        → Move them to Core 0-3"
echo "  • migration threads      → CPU isolation needed"
echo "  • IRQ handlers           → Rebind IRQs away from Core $CORE_TARGET"
echo "  • 'powersave' governor   → Switch to 'performance'"
echo "  • RCU callbacks          → Boot with rcu_nocbs=$CORE_TARGET"
echo ""
