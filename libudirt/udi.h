/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _UDI_H
#define _UDI_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Definition of the userland debugger interface (UDI)

// UDI types
typedef uint64_t udi_address;
typedef uint32_t udi_length;
typedef uint32_t udi_request_type;
typedef uint32_t udi_response_type;
typedef uint32_t udi_event_type;
typedef uint32_t udi_data_type;

/*
 * UDI data type
 */
typedef enum {
    UDI_DATATYPE_INT16 = 0,
    UDI_DATATYPE_INT32,
    UDI_DATATYPE_LENGTH,
    UDI_DATATYPE_INT64,
    UDI_DATATYPE_ADDRESS,
    UDI_DATATYPE_BYTESTREAM // encoded as a length and a following byte stream
} udi_data_type_e;

/**
 * architecture of debuggee
 */
typedef enum {
    UDI_ARCH_X86,
    UDI_ARCH_X86_64
} udi_arch_e;

/**
 * The version of the protocol
 */
typedef enum {
    UDI_PROTOCOL_VERSION_1 = 1
} udi_version_e;

/**
 * The running state for a thread
 */
typedef enum {
    UDI_TS_RUNNING = 0,
    UDI_TS_SUSPENDED,
} udi_thread_state_e;

/**
 * Registers
 */
typedef enum {
    // X86 registers
    UDI_X86_MIN = 0,
    UDI_X86_GS,
    UDI_X86_FS,
    UDI_X86_ES,
    UDI_X86_DS,
    UDI_X86_EDI,
    UDI_X86_ESI,
    UDI_X86_EBP,
    UDI_X86_ESP,
    UDI_X86_EBX,
    UDI_X86_EDX,
    UDI_X86_ECX,
    UDI_X86_EAX,
    UDI_X86_CS,
    UDI_X86_SS,
    UDI_X86_EIP,
    UDI_X86_FLAGS,
    UDI_X86_ST0,
    UDI_X86_ST1,
    UDI_X86_ST2,
    UDI_X86_ST3,
    UDI_X86_ST4,
    UDI_X86_ST5,
    UDI_X86_ST6,
    UDI_X86_ST7,
    UDI_X86_MAX,

    //UDI_X86_64 registers
    UDI_X86_64_MIN,
    UDI_X86_64_R8,
    UDI_X86_64_R9,
    UDI_X86_64_R10,
    UDI_X86_64_R11,
    UDI_X86_64_R12,
    UDI_X86_64_R13,
    UDI_X86_64_R14,
    UDI_X86_64_R15,
    UDI_X86_64_RDI,
    UDI_X86_64_RSI,
    UDI_X86_64_RBP,
    UDI_X86_64_RBX,
    UDI_X86_64_RDX,
    UDI_X86_64_RAX,
    UDI_X86_64_RCX,
    UDI_X86_64_RSP,
    UDI_X86_64_RIP,
    UDI_X86_64_CSGSFS,
    UDI_X86_64_FLAGS,
    UDI_X86_64_ST0,
    UDI_X86_64_ST1,
    UDI_X86_64_ST2,
    UDI_X86_64_ST3,
    UDI_X86_64_ST4,
    UDI_X86_64_ST5,
    UDI_X86_64_ST6,
    UDI_X86_64_ST7,
    UDI_X86_64_XMM0,
    UDI_X86_64_XMM1,
    UDI_X86_64_XMM2,
    UDI_X86_64_XMM3,
    UDI_X86_64_XMM4,
    UDI_X86_64_XMM5,
    UDI_X86_64_XMM6,
    UDI_X86_64_XMM7,
    UDI_X86_64_XMM8,
    UDI_X86_64_XMM9,
    UDI_X86_64_XMM10,
    UDI_X86_64_XMM11,
    UDI_X86_64_XMM12,
    UDI_X86_64_XMM13,
    UDI_X86_64_XMM14,
    UDI_X86_64_XMM15,
    UDI_X86_64_MAX

} udi_register_e;

/* 
 * Request specification:
 *
 * +------------------+------------+----------------+
 * | udi_request_type | udi_length | void *argument |
 * +------------------+------------+----------------+
 *
 */
typedef enum
{
    UDI_REQ_CONTINUE = 0,
    UDI_REQ_READ_MEM,
    UDI_REQ_WRITE_MEM,
    UDI_REQ_READ_REGISTER,
    UDI_REQ_WRITE_REGISTER,
    UDI_REQ_STATE,
    UDI_REQ_INIT,
    UDI_REQ_CREATE_BREAKPOINT,
    UDI_REQ_INSTALL_BREAKPOINT,
    UDI_REQ_REMOVE_BREAKPOINT,
    UDI_REQ_DELETE_BREAKPOINT,
    UDI_REQ_THREAD_SUSPEND,
    UDI_REQ_THREAD_RESUME,
    UDI_REQ_NEXT_INSTRUCTION,
    UDI_REQ_SINGLE_STEP,
    UDI_REQ_MAX,
    UDI_REQ_INVALID,
} udi_request_type_e;

/*
 * Response specification
 *
 * +-------------------+------------------+------------+---------------------+
 * | udi_response_type | udi_request_type | udi_length | void *packed_data   |
 * +-------------------+------------------+------------+---------------------+
 */
typedef enum
{
    UDI_RESP_ERROR = 0,
    UDI_RESP_VALID,
    UDI_RESP_MAX
} udi_response_type_e;

/* Request-response payload definitions 
 * 
 * For all request-reponse pairs:
 *      if udi_response_type == ERROR:
 *              packed_data == error message (of type UDI_DATATYPE_BYTESTREAM)
 */

/* Init request and response
 *
 * Request arguments:
 *      None
 *
 * Response values:
 *      UDI_DATATYPE_INT32 - the UDI protocol version
 *      UDI_DATATYPE_INT32 - the architecture of the debuggee
 *      UDI_DATATYPE_INT32 - whether the debuggee is multi-thread capable
 *      UDI_DATATYPE_INT64 - the tid for the initial thread
 */

/* Continue request and response
 *
 * It is an error to send this request to a thread pipe
 *
 * Request arguments:
 *      UDI_DATATYPE_INT32 - signal to pass to the debuggee (0 for no signal)
 *
 * Response values:
 *      None
 */

/* Read memory request and response
 *
 * It is an error to send this request to a thread pipe
 *
 * Request arguments:
 *      UDI_DATATYPE_ADDRESS - virtual memory address to read from
 *      UDI_DATATYPE_LENGTH - # of bytes to read
 *
 * Response values:
 *      UDI_DATATYPE_BYTESTREAM - requested memory values of target process
 */

/* Write memory request and response
 *
 * It is an error to send this request to a thread pipe
 *
 * Request arguments:
 *      UDI_DATATYPE_ADDRESS - virtual memory address to write to
 *      UDI_DATATYPE_BYTESTREAM - bytes to write into target process
 *
 * Response values:
 *      None
 */

/* Read register request and response
 *
 * It is an error to send this request to a process pipe
 *
 * Request arguments:
 *      UDI_DATATYPE_INT32 - the id of the register to read (udi_register_e)
 *
 * Response values:
 *      UDI_DATATYPE_ADDRESS - the value stored in the register
 */

/* Write register request and response
 *
 * It is an error to send this request to a process pipe
 *
 * Request arguments:
 *      UDI_DATATYPE_INT32 - the id of the register to write (udi_register_e)
 *      UID_DATATYPE_ADDRESS - the value to write to the register
 *
 * Response values:
 *      None
 */

/* Breakpoint create request and response
 *
 * Request arguments:
 *      UDI_DATATYPE_ADDRESSS - virtual memory address to set the breakpoint
 *
 * Response values:
 *      None
 */

/* Breakpoint install request and response
 * 
 * Request arguments:
 *      UDI_DATATYPE_ADDRESS - virtual memory address of the breakpoint to install
 *
 * Response values:
 *      None
 */

/* Breakpoint delete request and response
 *
 * Request arguments:
 *      UDI_DATATYPE_ADDRESS - virtual memory address of the breakpoint to remove
 *
 * Response values:
 *      None
 */

/* Breakpoint remove request and response
 *
 * Request arguments:
 *      UDI_DATATYPE_ADDRESS - virtual memory address of the breakpoint to disable
 *
 * Response values:
 *      None
 */

/**
 * Thread suspend request and response
 *
 * It is an error to send this request to the process request pipe
 *
 * Request arguments:
 *      None
 *
 * Response values:
 *      None
 */

/**
 * Thread resume request and response
 *
 * It is an error to send this request to the process request pipe
 *
 * Request arguments:
 *      None
 *
 * Response values:
 *      None
 */

/**
 * Thread next instruction request and response
 *
 * It is an error to send this request to the process request pipe
 *
 * Request arguments:
 *      None
 *
 * Response values:
 *      UDI_DATATYPE_ADDRESS - the address of the next instruction to be executed
 */

/**
 * Thread singlestep setting request and response
 *
 * It is an error to send this request to the process request pipe
 *
 * Request arguments:
 *      UDI_DATATYPE_INT16 - non-zero to enable single stepping -- not include to just retrieve the value
 *
 * Response values:
 *      UDI_DATATYPE_INT16 - the previous setting value
 */

/* 
 * State request and response
 *
 * Request arguments:
 *      None
 *
 * Response values:
 *      UDI_DATATYPE_INT64 - thread id 1
 *      UDI_DATATYPE_INT16 - thread state 1
 *      ... (continued for number of threads in process)
 */

/* Event specification
 *
 * Events occur asynchronously 
 * -- that is they never directly occur in response to requests.
 */

typedef enum
{
    UDI_EVENT_ERROR = 0,
    UDI_EVENT_SIGNAL,
    UDI_EVENT_BREAKPOINT,
    UDI_EVENT_THREAD_CREATE,
    UDI_EVENT_THREAD_DEATH,
    UDI_EVENT_PROCESS_EXIT,
    UDI_EVENT_PROCESS_FORK,
    UDI_EVENT_PROCESS_EXEC,
    UDI_EVENT_SINGLE_STEP,
    UDI_EVENT_PROCESS_CLEANUP,
    UDI_EVENT_MAX,
    UDI_EVENT_UNKNOWN
} udi_event_type_e;

/*
 * Event data
 *
 * +----------------+-----------+------------+------------+
 * | udi_event_type | thread id | udi_length | void *data |
 * +----------------+-----------+------------+------------+
 */

/* the thread id is of type UDI_DATATYPE_INT64 */
#define UDI_SINGLE_THREAD_ID 0xC0FFEEABC

/*
 * Error event data
 *
 * UDI_DATATYPE_BYTESTREAM - error message
 */

/*
 * Signal event data
 *
 * UDI_DATATYPE_ADDRESS - virtual address where the signal occurred
 * UDI_DATATYPE_INT32   - the signal number that occurred
 */

/*
 * Breakpoint event data
 *
 * UDI_DATATYPE_ADDRESS - virtual addresss where the breakpoint occurred
 */

/**
 * Thread create event data
 *
 * UDI_DATATYPE_INT64 - the thread id for the newly created thread
 */

/**
 * Thread death event data
 */

/*
 * Process exit data
 *
 * UDI_DATATYPE_INT32   - the exit code from the process exit
 */

/*
 * Process fork data
 *
 * UDI_DATATYPE_INT32   - the process id for the new process
 */

/*
 * Thread single step event
 */

/*
 * Process cleanup data
 */

#ifdef __cplusplus
} // extern C
#endif

#endif
