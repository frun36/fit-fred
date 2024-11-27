## Read-write structure
Read-write operations require knowledge of the register map and its internal structure (e.g., bit fields). FIT FRED removes the responsibility from the WinCC system to provide this data, as it manages all information about the board memory layout. Consequently, WinCC only needs to know the parameter name.

The table below shows the WinCC request format. WinCC can send a series of READ or WRITE transactions (READ and WRITE requests cannot be sent together). This format is also used internally in FRED to create requests for the electronic components. In a few cases, services use special words for additional commands, but this will not be discussed here.

| Command name | First argument | Second argument |
|------------------|------------------|------------------|
| READ     | parameter name     | -    |
| WRITE     | parameter name     | physical value     |

After each WRITE transaction, a READ transaction is executed to ensure that the value was successfully written. The response to both READ and WRITE operations follows the same structure.

```
[PARAMETER_NAME],[VALUES],
[PARAMETER_NAME],[VALUES],
...
```

