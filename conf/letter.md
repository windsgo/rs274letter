# Letter Config
```
A total of 52 G-codes are implemented.

The groups are:
group  0 = {g4,g10,g28,g30,g52,g53,g92,g92.1,g92.2,g92.3} - NON-MODAL
            dwell, setup, return to ref1, return to ref2,
            local coordinate system, motion in machine coordinates,
            set and unset axis offsets
group  1 = {g0,g1,g2,g3,g33,g33.1,g38.2,g38.3,g38.4,g38.5,g73,g76,g80,
            g81,g82,g83,g84,g85,g86,g87,g88,g89} - motion
group  2 = {g17,g17.1,g18,g18.1,g19,g19.1}   - plane selection
group  3 = {g90,g91}       - distance mode
group  4 = {g90.1,g91.1}   - arc IJK distance mode
group  5 = {g93,g94,g95}   - feed rate mode
group  6 = {g20,g21}       - units
group  7 = {g40,g41,g42}   - cutter diameter compensation
group  8 = {g43,g49}       - tool length offset
group 10 = {g98,g99}       - return mode in canned cycles
group 12 = {g54,g55,g56,g57,g58,g59,g59.1,g59.2,g59.3} - coordinate system
group 13 = {g61,g61.1,g64} - control mode (path following)
group 14 = {g96,g97}       - spindle speed mode
group 15 = {G07,G08}       - lathe diameter mode
```