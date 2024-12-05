# `RESET`

## Overview
`RESET` is a simple service to perform **one reset command at a time**. It handles reset' bit setting and clearing to make sure that current command will not interfere with the next one.

## WinCC request

Request should contains only of the commands listed below:
- GBT_RESET
- GBT_RESET_DATA_COUNTERS
- GBT_START_OF_EMULATION
- GBT_RESET_ORBIT_SYNC
- GBT_RESET_READOUT_FSM
- GBT_RESET_RX_ERROR
- GBT_RESET_RX_PHASE_ERROR
- GBT_RESET_ERROR_REPORT_FIFO

## Response to WinCC

If reset flag is set and cleared without interruption, then `SUCCESS` message is published

## Error handling
In case of communication failure with ALF or electronic, or invalid request the error message with description is published.