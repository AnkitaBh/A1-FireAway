# Wildfire_WSN
Network STM32 library for capstone project 

Procedure to run GW node:
1. In Wildfire_WSN/ directory
2. make clean; make
3. sudo ./run_gw_router

Tasks left for GW code:
1. calculate the time by which all nodes should have received the schedule
    1. add this to the SCH packet,
    2. nodes will receive the SCH packet and calculate the time to sleep before running the DATA phase
    3. Note that this means that there can only be N packet errors in the SCH phase (N retransmissions)
2. JSON format to feed to web app
    1. cat this to a file after receiving data (after  Phase 2, Phase 4)


