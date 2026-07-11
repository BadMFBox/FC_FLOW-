#!/usr/bin/env python3
import pandas as pd
import sys

# Load data
df = pd.read_csv('governor_correlation.csv')

# Find high-latency events (p99+)
p99_latency = df['latency_us'].quantile(0.99)
high_latency = df[df['latency_us'] > p99_latency]

print("========================================================")
print("   WALT TAMPERING CORRELATION ANALYSIS                 ")
print("========================================================\n")

print(f"Total validations: {len(df)}")
print(f"p99 latency threshold: {p99_latency:.2f} μs")
print(f"High-latency events: {len(high_latency)}\n")

if len(high_latency) > 0:
    print("High-Latency Events with Governor State:")
    print(high_latency[['iteration', 'latency_us', 'cpu_freq_khz']].to_string(index=False))
    print("")
    
    # Check if frequency dropped during high latency
    avg_freq_normal = df[df['latency_us'] <= p99_latency]['cpu_freq_khz'].mean()
    avg_freq_spike = high_latency['cpu_freq_khz'].mean()
    
    print(f"Average frequency during normal latency: {avg_freq_normal:.0f} kHz")
    print(f"Average frequency during high latency: {avg_freq_spike:.0f} kHz")
    print("")
    
    if avg_freq_spike < avg_freq_normal * 0.85:
        print("⚠ EVIDENCE OF WALT TAMPERING:")
        print(f"  Frequency dropped by {((avg_freq_normal - avg_freq_spike) / avg_freq_normal * 100):.1f}%")
        print("  during high-latency events.")
        print("")
        print("CONCLUSION: Governor is throttling crypto validation.")
        print("This is either malicious configuration or aggressive power saving.")
    else:
        print("✓ No correlation between frequency and latency.")
        print("  Jitter likely caused by something else (scheduler, cache, etc.)")
else:
    print("No high-latency events detected. System running clean.")

print("\n========================================================")
