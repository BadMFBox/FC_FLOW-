# FuelTank

Shared fuel reserve. All sectors draw from here on demand.

## Tiers
- WARM     — active, in RAM, mlock'd
- COLD     — inactive, on storage
- REACTIVE — promoted from cold on threat detection
- STABLE   — demoted from warm after clean cycles

## Status: STUB
