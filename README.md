# (Input-Queued) Switch Scheduling Simulator


## Algorithms under Exploration

### SB-QPS

+ [X] Oblivious Half & Half SB-QPS:
    + First half, normal QPS-1
    + Second half, allow (the second longest proposal at each output port) to try previous time slot in the same frame but do not check whether there are available ones
+ [X] Availability-Aware Half & Half SB-QPS:
    + Similar as Oblivious Half & Half SQ-QPS, but checking availability (not just limited to the second longest, all proposals can try previous time slots. However, when contention happens, longer proposals win)
+ [ ] Adaptive SB-QPS:
    + A simple extension to AA-H&H-SB-QPS: allow trying previous ones as long as there are available ones

