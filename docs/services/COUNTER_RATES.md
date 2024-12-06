# `COUNTER_RATES`

## Overview
The `COUNTER_RATES` service is responsible for reading the most up-to-date counter values, and publishing them along with rate values it has calculated. The counters are read from the FIFO (address 0x0100 on TCM and each PM), and the rate is calculated as the difference between two most recent counter values divided by the `COUNTER_READ_INTERVAL`.

## Response
The response sent to WinCC after each readout attempt is as follows:
```
READ_INTERVAL,[READ_INTERVAL_STATE],[VAL_SECONDS]\n
FIFO_STATE,[FIFO_STATE],[FIFO_LOAD]\n
FIFO_READ_RESULT,[FIFO_READ_RESULT]\n
COUNTERS,[COMMA,SEPARATED,VALUES]\n
RATES,[COMMA,SEPARATED,VALUES]\n
PREV_ELAPSED,[ELAPSED]\n
[info about executed commands (START, STOP, RESET)]
```

The consecutive counter values for the TCM are: 
- `TRIGGER_5_COUNTER` 
- `TRIGGER_4_COUNTER` 
- `TRIGGER_3_COUNTER` 
- `TRIGGER_2_COUNTER` 
- `TRIGGER_1_COUNTER`
- `BACKGROUND_0_COUNTER`
- `BACKGROUND_1_COUNTER`
- `BACKGROUND_2_COUNTER`
- `BACKGROUND_3_COUNTER`
- `BACKGROUND_4_COUNTER`
- `BACKGROUND_5_COUNTER`
- `BACKGROUND_6_COUNTER`
- `BACKGROUND_7_COUNTER`
- `BACKGROUND_8_COUNTER`
- `BACKGROUND_9_COUNTER`

And for the PM:
- `CH01_CTR_CFD`
- `CH01_CTR_TRG`
- `CH02_CTR_CFD`
- `CH02_CTR_TRG`
- `CH03_CTR_CFD`
- `CH03_CTR_TRG`
- `CH04_CTR_CFD`
- `CH04_CTR_TRG`
- `CH05_CTR_CFD`
- `CH05_CTR_TRG`
- `CH06_CTR_CFD`
- `CH06_CTR_TRG`
- `CH07_CTR_CFD`
- `CH07_CTR_TRG`
- `CH08_CTR_CFD`
- `CH08_CTR_TRG`
- `CH09_CTR_CFD`
- `CH09_CTR_TRG`
- `CH10_CTR_CFD`
- `CH10_CTR_TRG`
- `CH11_CTR_CFD`
- `CH11_CTR_TRG`
- `CH12_CTR_CFD`
- `CH12_CTR_TRG`

