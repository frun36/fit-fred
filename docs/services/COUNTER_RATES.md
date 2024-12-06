# `COUNTER_RATES`

## Overview
The `COUNTER_RATES` service is responsible for reading the most up-to-date counter values, and publishing them along with rate values it has calculated. The counters are read from the FIFO (address 0x0100 on TCM and each PM), and the rate (in Hz) is calculated as the difference between two most recent counter values divided by the `COUNTER_READ_INTERVAL`.

## Response
The response sent to WinCC after each readout attempt is as follows:
```
READ_INTERVAL,[READ_INTERVAL_STATE],[VAL_SECONDS]\n
FIFO_STATE,[FIFO_STATE],[FIFO_LOAD]\n
FIFO_READ_RESULT,[FIFO_READ_RESULT]\n
COUNTERS,[COMMA,SEPARATED,VALUES]\n
RATES,[COMMA,SEPARATED,VALUES,Hz]\n
PREV_ELAPSED,[ELAPSED]\n
[info about executed commands (START, STOP, RESET)]
```
If counters or rates are unavailable, `-` is published instear of the comma separated values.

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

## Requests
The service inherits `LoopingFitIndefiniteMapi`, so it supports the `START` and `STOP` commands. Additionally, a `RESET` command can be sent, to reset counters. 

> [!NOTE]
> Command handling is performed only after a successful FIFO readout, to minimise the chance of a reset occurring inbetween FIFO loads. There is still, however, a chance that this might happen (tests will show) - then, the reported rate values will be 2x higher for the duration of one `COUNTER_READ_INTERVAL`.

## Service operation
The `processExecution` method is executed every `0.5 * COUNTER_READ_INTERVAL` and is divided into the following stages:
- sleep handling (from `LoopingFitIndefiniteMapi`)
- read interval check
    - try to fetch the value from electronics - if unsuccessful or the value is invalid, error, wait a while and return
    - if the value is 0 (`ReadIntervalState::Disabled`), error (since software timer mode is unsupported), wait a while and return
    - see if the read interval was changed (`ReadIntervalState::Changed`) or not (`ReadIntervalState::Ok`)
- fifo readout handling
    - read `FIFO_LOAD` and evaluate FIFO state
    - `FifoState::Outdated` - FIFO is full (potential loss of newest data), or read interval was changed - clear FIFO, no counters or rates
    - `FifoState::BoardError` - `FIFO_LOAD == 0xFFFFFFFF`, couldn't communicate with PM - error, return
    - `FifoState::Empty` - no data in FIFO (expected half of the time), publish previous values
    `FifoState::Single` - single set of counters in FIFO, read them, calculate rates based on difference with previous values stored in FRED, publish counters and rates
    - `FifoState::Multiple` - multiple set of counters in FIFO, read them, calculate rates based on difference between two newest sets, publish counters and rates
    - `FifoState::Partial` - readout performed in the middle of FIFO fill, print warning. Tests show, that by the time the next IPbus packet has arrived, the FIFO will have already been filled with the complete set of counters - round up to the nearest multiple of the number of counters and proceed as with `Multiple`/`Single`
- `START`/`STOP`/`RESET` command handling
    - `START`/`STOP` handled in `LoopingFitIndefiniteMapi::handleSleepAndWake`
    - `RESET`
        - To compensate for the fact that a reset can occur anywhere inbetween FIFO loads, a direct counter readout is performed (registers `0x70-0x7E` for TCM, `0xC0-0xD7` for PM).
        - Performed after successful FIFO readout - minimises chance of invalid rate calculation afterwards

## Behaviour in special situations
- `COUNTER_READ_INTERVAL` change
    - clear FIFO - no rates/counters
    - [empty FIFO]
    - read FIFO - no rates can be calculated
    - [empty FIFO]
    - read FIFO - counters + rates
    - total of `2*NEW_COUNTER_READ_INTERVAL` break from publishing newest rates
- reset counters
    - can result in some instability in the published rate values (tests show they usually are a bit smaller that they should be, it's potentially possible that they will be reported as 2x too large for one `COUNTER_READ_INTERVAL`)
- reset system
    - sets `COUNTER_READ_INTERVAL` to 0 - detected by the `STATUS` service