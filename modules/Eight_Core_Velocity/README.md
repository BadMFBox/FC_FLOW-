# Eight_Core_Velocity (8CV)

FC_FLOW's core engine. 8 concurrent encode/decode loops at 3.7Hz.

## Spec
- 8 jthread loops — one per core
- 1.8 units per tick per core — 14.4 total combined
- Tick interval: 270ms (3.7Hz)
- Zero heap allocation in tick loop
- clock_gettime(CLOCK_MONOTONIC) for all timing
- Each tick: CrossBolt fires, FuelTank draws, Monitus witnesses

## Status: STUB
