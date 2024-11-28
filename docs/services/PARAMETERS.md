# `PARAMETERS`

## Overview

Parameters provides interface for simple read/write operations on one or many parameters excluding FIFO registers. 

## WinCC Request

WinCC request should be formatted in a standard way described [here](../others/ReadWriteWinCC.md).

## Response to WinCC

After a successful operation, `PARAMETERS` sends the list of parameter values involved in the transaction. For write transactions, the value is read from the electronics after the write operation.

```
[PARAMETER_A],[VALUE]
[PARAMETER_B],[VALUE]
...
```

## Error handling
- In case of failure in WinCC request parsing, the error is published immeadiately.

- In case of failure in ALF response parsing, the error message is published along with the part of data that was successfuly retrieved from message.

## Code organisation

`Parameters` class inherits from the Mapi class. Translation of WinCC requests to ALF requests and parsing ALF response is performed using `BoardCommunicationHandler`, holding pointer to board to which service belongs.