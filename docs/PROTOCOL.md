# UDI Wire Protocol

The UDI protocol encodes messages using [CBOR](cbor.io). There are three types
of messages: `request`, `response`, or `event`. The message type is determined
by the source or destination of the message.

## Requests

A request is composed of two data items:

1. An unsigned, 16-bit integer value that defines the type of the request
2. A map containing pairs that are defined by the type of the request

The possible values for the request type are in the table below:

| Request Type        | Value |
| ------------        | ----- |
| invalid             | 0     |
| continue            | 1     |
| read memory         | 2     |
| write memory        | 3     |
| read register       | 4     |
| write register      | 5     |
| state               | 6     |
| init                | 7     |
| create breakpoint   | 8     |
| install breakpoint  | 9     |
| remove breakpoint   | 10    |
| delete breakpoint   | 11    |
| thread suspend      | 12    |
| thread resume       | 13    |
| next instruction    | 14    |
| single step         | 15    |

## Responses

A response is composed of three data items. The first two items always take on the following form:

1. An unsigned, 16-bit integer value that defines the type of the response
2. An unsigned, 16-bit integer value that defines the type of the corresponding request

The following is a list of response types and corresponding values:

| Response Type | Value |
| ------------- | ----- |
| valid         | 0     |
| error         | 1     |

When the response type is `error`, the third data item is a map with the following pair:

| Key   | Value                    | Description      |
| ----- | ------------------------ | -------------    |
| msg   | text string              | An error message |

Responses with a response type of `valid` have a third data item that is a map. The pairs in
the map are dictated by the type of the request.

## Events

An event is composed of three data items. The first two items always take on the following
form:

1. An unsigned, 16-bit integer value that defines the type of the event.
2. An unsigned, 64-bit integer value that identifies the id of the thread that triggered the event.

The following is a list of event types and corresponding values:

| Event Type      | Value |
| ----------      | ----- |
| unknown         | 0 |
| error           | 1 |
| signal          | 2 |
| breakpoint      | 3 |
| thread create   | 4 |
| thread death    | 5 |
| process exit    | 6 |
| process fork    | 7 |
| process exec    | 8 |
| single step     | 9 |
| process cleanup | 10 |

The third data item is a map and its pairs are defined by the event type.

## Request and Response Data

**continue**

Continues a debuggee with the specified signal. It is an error to send this
request to a thread.

_Inputs_

- `sig`: The signal to pass to the debuggee (0 for no signal) as an unsigned, 32-bit integer.

_Outputs_

No outputs.

**read memory**

Reads debuggee memory. It is an error to send this request to a thread.

_Inputs_

- `addr`: The virtual memory address to read from as an unsigned, 64-bit integer
- `len`: The length of bytes to read as an unsigned, 32-bit integer

_Outputs_

- `data`: The data read as a byte string.

**write memory**

Writes debuggee memory. It is an error to send this request to a thread.

_Inputs_

- `addr`: The virtual memory address to write to as an unsigned, 64-bit integer
- `data`: The data to write as a byte string.

_Outputs_

No outputs.

**read register**

Reads a register from a thread's current context. It is an error to send this request to
a process.

_Inputs_

- `reg`: The register to read as an unsigned, 16-bit integer

_Outputs_

- `value`: The register value as an unsigned, 64-bit integer

**write register**

Writes a register in a thread's current context. It is an error to send this request to
a process.

_Inputs_

- `reg`: The register to write as an unsigned, 16-bit integer
- `value`: The value to write as an unsigned, 64-bit integer

_Outputs_

No outputs.

**state**

Retrieves the state of the threads in a process. The output differs depending on whether this
request was sent to the process or a single thread.

_Inputs_

No inputs.

_Outputs_

If the recipient was the process:

- `states`: An array of maps with the following keys:
  - `tid`: Thread id as an unsigned, 64-bit integer
  - `state`: Thread state as an unsigned, 16-bit integer

If the recipient was the thread:

- `tid`: Thread id as an unsigned, 64-bit integer
- `state`: Thread state as an unsigned, 16-bit integer

**init**

Complete the initialization of a debuggee.

_Inputs_

No inputs.

_Outputs_

- `v`: The UDI protocol version as an unsigned, 32-bit integer
- `arch`: The architecture of the debuggee as an unsigned, 16-bit integer
- `mt`: `true` when the debuggee is multithread capable
- `tid`: The tid for the initial thread as an unsigned, 64-bit integer

**create breakpoint**

Creates a breakpoint

_Inputs_

- `addr`: The address of the breakpoint as an unsigned, 64-bit integer

_Outputs_

No outputs.

**install breakpoint**

Install the breakpoint into memory.

_Inputs_

- `addr`: The address of the breakpoint as an unsigned, 64-bit integer

_Outputs_

No outputs.

**remove breakpoint**

Removes the breakpoint from memory.

_Inputs_

- `addr`: The address of the breakpoint as an unsigned, 64-bit integer

_Outputs_

No outputs.

**delete breakpoint**

Delete the breakpoint

_Inputs_

- `addr`: The address of the breakpoint as an unsigned, 64-bit integer

_Outputs_

No outputs.

**thread suspend**

Mark a thread suspended. It is an error to send this request to a process.

_Inputs_

No inputs.

_Outputs_

No outputs.

**thread resume**

Mark a thread resumed. It is an error to send this request to a process.

_Inputs_

No inputs.

_Outputs_

No outputs.

**next instruction**

Retrieves the address of the next instruction to be executed.

_Inputs_

No inputs.

_Outputs_

- `addr`: The address of the next instruction as an unsigned, 64-bit integer

**single step**

Change the single step setting for a thread. It is an error to send this request to
a process.

_Inputs_

- `value`: `true` when single stepping is to be enabled for the thread

_Outputs_

- `value`: The previous setting as a boolean

## Event Data

**error**

- `msg`: The error message as a text string

**signal**

- `addr`: The virtual address where the signal occurred as an unsigned, 64-bit integer
- `sig`: The signal number as an unsigned integer

**breakpoint**

- `addr`: The virtual address where the breakpoint occurred as an unsigned, 64-bit integer

**thread create**

- `tid`: The thread id for the newly created thread as an unsigned, 64-bit integer

**thread death**

No data.

**process exit**

- `code`: The exit code from the process exit as a signed, 32-bit integer.

**process fork**

- `pid`: The process id for the new process as an unsigned, 32-bit integer.

**process exec**

- `path`: The path to the new executable image as a text string
- `argv`: The arguments to the new execution as an array of text strings
- `envp`: The environment for the new execution as an array of text strings

**single step**

No data.

**process cleanup**

No data.
