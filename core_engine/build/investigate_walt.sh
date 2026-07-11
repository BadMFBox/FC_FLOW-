#!/usr/bin/env bash
set -euo pipefail

echo "========================================================="
echo "   WALT INVESTIGATION — Gathering Evidence from Arch    "
echo "========================================================="
echo ""

echo "[*] Checking if sysfs is accessible from proot..."
if [ -d /sys/devices/system/cpu ]; then
    echo "✓ /sys is mounted and accessible"
else
    echo "✗ /sys not accessible from proot"
    exit 1
fi

echo ""
echo "[*] Detecting CPU topology..."
for cpu in /sys/devices/system/cpu/cpu[0-9]*; do
    if [ -d "$cpu/cpufreq" ]; then
        cpu_num=$(basename "$cpu" | sed 's/cpu//')
        cur=$(cat "$cpu/cpufreq/scaling_cur_freq" 2>/dev/null || echo "N/A")
        max=$(cat "$cpu/cpufreq/scaling_max_freq" 2>/dev/null || echo "N/A")
        gov=$(cat "$cpu/cpufreq/scaling_governor" 2>/dev/null || echo "N/A")
        
        echo "CPU $cpu_num: $cur kHz / $max kHz (governor: $gov)"
    fi
done

echo ""
echo "[*] Checking for WALT-specific tunables on big cores (4-7)..."
for cpu in 4 5 6 7; do
    WALT_DIR="/sys/devices/system/cpu/cpu$cpu/cpufreq/walt"
    
    if [ -d "$WALT_DIR" ]; then
        echo ""
        echo "CPU $cpu WALT Configuration:"
        
        for tunable in hispeed_freq hispeed_load target_load_thresh up_rate_limit_us down_rate_limit_us; do
            if [ -f "$WALT_DIR/$tunable" ]; then
                value=$(cat "$WALT_DIR/$tunable")
                echo "  $tunable: $value"
            fi
        done
        
        # Calculate if hispeed is malicious
        MAX=$(cat /sys/devices/system/cpu/cpu$cpu/cpufreq/scaling_max_freq)
        HISPEED=$(cat "$WALT_DIR/hispeed_freq" 2>/dev/null || echo "0")
        
        if [ "$HISPEED" -gt 0 ]; then
            HISPEED_PCT=$((HISPEED * 100 / MAX))
            echo "  hispeed_freq is $HISPEED_PCT% of max"
            
            if [ $HISPEED_PCT -lt 60 ]; then
                echo "  ⚠ SUSPICIOUS: hispeed_freq artificially low (expected >80%)"
            fi
        fi
    else
        echo "CPU $cpu: No WALT tunables directory"
    fi
done

echo ""
echo "========================================================="
echo "   EVIDENCE COLLECTION                                  "
echo "========================================================="

# Dump full governor state to file for analysis
OUTPUT="walt_evidence_$(date +%Y%m%d_%H%M%S).txt"
echo "Collecting full system state to: $OUTPUT"
echo ""

{
    echo "=== CPU Topology ==="
    lscpu
    echo ""
    
    echo "=== Governor States ==="
    for cpu in 0 1 2 3 4 5 6 7; do
        if [ -d /sys/devices/system/cpu/cpu$cpu/cpufreq ]; then
            echo "CPU $cpu:"
            cat /sys/devices/system/cpu/cpu$cpu/cpufreq/scaling_*
            echo ""
        fi
    done
    
    echo "=== WALT Tunables (if present) ==="
    for cpu in 4 5 6 7; do
        WALT_DIR="/sys/devices/system/cpu/cpu$cpu/cpufreq/walt"
        if [ -d "$WALT_DIR" ]; then
            echo "CPU $cpu:"
            cat "$WALT_DIR/"*
            echo ""
        fi
    done
    
    echo "=== Thermal State ==="
    for thermal in /sys/class/thermal/thermal_zone*/temp; do
        if [ -f "$thermal" ]; then
            zone=$(dirname "$thermal")
            type=$(cat "$zone/type" 2>/dev/null || echo "unknown")
            temp=$(cat "$thermal")
            echo "$type: $temp"
        fi
    done
    
} > "$OUTPUT"

echo "✓ Evidence saved to: $OUTPUT"
echo ""
echo "Next steps:"
echo "  1. Review $OUTPUT for anomalies"
echo "  2. Run FC_FLOW bench with timestamp logging"
echo "  3. Correlate jitter spikes with governor state changes"
echo "  4. If evidence confirms tampering: add detection to FC_FLOW"
echo "========================================================="
