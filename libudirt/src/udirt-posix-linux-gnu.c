/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <ucontext.h>
#include <stdint.h>
#include <thread_db.h>
#include <sys/procfs.h>
#include <link.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#include "udirt-posix.h"

// exported constants //

// library wrapping
void *UDI_RTLD_NEXT = RTLD_NEXT;

// register interface

// x86
#ifndef REG_GS
#define REG_GS -1
#endif
int X86_GS_OFFSET = REG_GS;

#ifndef REG_FS
#define REG_FS -1
#endif
int X86_FS_OFFSET = REG_FS;

#ifndef REG_ES
#define REG_ES -1
#endif
int X86_ES_OFFSET = REG_ES;

#ifndef REG_DS
#define REG_DS -1
#endif
int X86_DS_OFFSET = REG_DS;

#ifndef REG_EDI
#define REG_EDI -1
#endif
int X86_EDI_OFFSET = REG_EDI;

#ifndef REG_ESI
#define REG_ESI -1
#endif
int X86_ESI_OFFSET = REG_ESI;

#ifndef REG_EBP
#define REG_EBP -1
#endif
int X86_EBP_OFFSET = REG_EBP;

#ifndef REG_ESP
#define REG_ESP -1
#endif
int X86_ESP_OFFSET = REG_ESP;

#ifndef REG_EBX
#define REG_EBX -1
#endif
int X86_EBX_OFFSET = REG_EBX;

#ifndef REG_EDX
#define REG_EDX -1
#endif
int X86_EDX_OFFSET = REG_EDX;

#ifndef REG_ECX
#define REG_ECX -1
#endif
int X86_ECX_OFFSET = REG_ECX;

#ifndef REG_EAX
#define REG_EAX -1
#endif
int X86_EAX_OFFSET = REG_EAX;

#ifndef REG_CS
#define REG_CS -1
#endif
int X86_CS_OFFSET = REG_CS;

#ifndef REG_SS
#define REG_SS -1
#endif
int X86_SS_OFFSET = REG_SS;

#ifndef REG_EIP
#define REG_EIP -1
#endif
int X86_EIP_OFFSET = REG_EIP;

#ifndef REG_EFL
#define REG_EFL -1
#endif
int X86_FLAGS_OFFSET = REG_EFL;

// x86_64

#ifndef REG_R8
#define REG_R8 -1
#endif
int X86_64_R8_OFFSET = REG_R8;

#ifndef REG_R9
#define REG_R9 -1
#endif
int X86_64_R9_OFFSET = REG_R9;

#ifndef REG_R10
#define REG_R10 -1
#endif
int X86_64_R10_OFFSET = REG_R10;

#ifndef REG_R11
#define REG_R11 -1
#endif
int X86_64_R11_OFFSET = REG_R11;

#ifndef REG_R12
#define REG_R12 -1
#endif
int X86_64_R12_OFFSET = REG_R12;

#ifndef REG_R13
#define REG_R13 -1
#endif
int X86_64_R13_OFFSET = REG_R13;

#ifndef REG_R14
#define REG_R14 -1
#endif
int X86_64_R14_OFFSET = REG_R14;

#ifndef REG_R15
#define REG_R15 -1
#endif
int X86_64_R15_OFFSET = REG_R15;

#ifndef REG_RDI
#define REG_RDI -1
#endif
int X86_64_RDI_OFFSET = REG_RDI;

#ifndef REG_RSI
#define REG_RSI -1
#endif
int X86_64_RSI_OFFSET = REG_RSI;

#ifndef REG_RBP
#define REG_RBP -1
#endif
int X86_64_RBP_OFFSET = REG_RBP;

#ifndef REG_RBX
#define REG_RBX -1
#endif
int X86_64_RBX_OFFSET = REG_RBX;

#ifndef REG_RDX
#define REG_RDX -1
#endif
int X86_64_RDX_OFFSET = REG_RDX;

#ifndef REG_RAX
#define REG_RAX -1
#endif
int X86_64_RAX_OFFSET = REG_RAX;

#ifndef REG_RCX
#define REG_RCX -1
#endif
int X86_64_RCX_OFFSET = REG_RCX;

#ifndef REG_RSP
#define REG_RSP -1
#endif
int X86_64_RSP_OFFSET = REG_RSP;

#ifndef REG_RIP
#define REG_RIP -1
#endif
int X86_64_RIP_OFFSET = REG_RIP;

#ifndef REG_CSGSFS
#define REG_CSGSFS -1
#endif
int X86_64_CSGSFS_OFFSET = REG_CSGSFS;

int X86_64_FLAGS_OFFSET = REG_EFL;

/**
 * Searches for a symbol in the object that is loaded at the specified address
 *
 * @param object_load_addr the address at which the object is loaded
 * @param sym_name the symbol name
 * @param loaded_offset the loaded offset of the object in the current address space
 * @param addr output parameter -- the address of the symbol
 *
 * @return 0 on success; non-zero on failure
 */
static
int search_for_sym(void *object_load_addr, const char *sym_name, unsigned long loaded_offset, 
        unsigned long *addr) 
{

    ElfW(Ehdr) *ehdr = (ElfW(Ehdr) *)object_load_addr;

    // Do some sanity checking to make sure the pointer is valid
    if (    ehdr->e_ident[EI_MAG0] != ELFMAG0 
         || ehdr->e_ident[EI_MAG1] != ELFMAG1
         || ehdr->e_ident[EI_MAG2] != ELFMAG2
         || ehdr->e_ident[EI_MAG3] != ELFMAG3
       )
    {
        udi_printf("invalid ELF identifier in header: %s\n", ehdr->e_ident);
        return -1;
    }

    if (ehdr->e_shoff == 0 || ehdr->e_shnum == 0) {
        udi_printf("%s\n", "section header format not currently supported");
        return -1;
    }

    ElfW(Shdr) *shdrs = (ElfW(Shdr) *)(object_load_addr + ehdr->e_shoff);

    uint16_t i;
    for (i = 0; i < ehdr->e_shnum; ++i) {
        ElfW(Shdr) *shdr = &shdrs[i];

        switch (shdr->sh_type) {
            case SHT_SYMTAB:
            case SHT_DYNSYM:
            {
                ElfW(Shdr) *link_hdr = &shdrs[shdr->sh_link];
                char *names = (char *)(object_load_addr + link_hdr->sh_offset);

                ElfW(Sym) *syms = (ElfW(Sym) *)(object_load_addr + shdr->sh_offset);
                uint32_t num_syms = shdr->sh_size / sizeof(ElfW(Sym));

                uint32_t j;
                for (j = 0; j < num_syms; ++j) {
                    ElfW(Sym) *sym = &syms[j];

                    if (sym->st_name != 0) {
                        if (strcmp(sym_name, &names[sym->st_name]) == 0) {
                            *addr = loaded_offset + sym->st_value;
                            return 0;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    udi_printf("failed to locate symbol %s\n", sym_name);
    return -1;
}

/**
 * Searches the specified symbol in the specified object
 *
 * @param object_path the absolute path to the object
 * @param sym_name the name of the symbol
 * @param loaded_offset the offset of the object in the current address space
 * @param addr the address of the symbol -- output parameter
 *
 * @return 0 on success; non-zero otherwise
 */
static
int search_for_sym_in_obj(const char *object_path, const char *sym_name, unsigned long loaded_offset, 
        unsigned long *addr) 
{
    // temporarily load the full object to get access to the symbols
    int obj_fd = open(object_path, O_RDONLY);
    if (obj_fd == -1) {
        udi_printf("failed to open %s: %s\n", object_path, strerror(errno));
        return -1;
    }

    int search_result = 0;
    do {
        struct stat obj_stat;
        if ( fstat(obj_fd, &obj_stat) != 0 ) {
            udi_printf("failed to stat %s: %s\n", object_path, strerror(errno));
            search_result = -1;
            break;
        }

        void *object_ptr = mmap(NULL, obj_stat.st_size, PROT_READ, MAP_SHARED, obj_fd, 0);
        if (object_ptr == NULL) {
            udi_printf("failed to load %s\n", object_path);
            search_result = -1;
            break;
        }

        if ( (search_result = search_for_sym(object_ptr, sym_name, loaded_offset, addr)) != 0) {
            udi_printf("failed to locate symbol %s in %s\n",
                    sym_name, object_path);
            search_result = -1;
        }

        if ( munmap(object_ptr, obj_stat.st_size) != 0 ) {
            udi_printf("failed to unload %s: %s\n", object_path, strerror(errno));
            search_result = -1;
        }
    }while(0);

    close(obj_fd); // explicitly ignore errors

    return search_result;
}

/**
 * Gets the address of the specified symbol
 *
 * @param object_name the object containing the symbol
 * @param sym_name the symbol
 * @param addr output parameter -- address for the symbol
 *
 * @return 0 on success; non-zero on failure
 */
static
int get_sym_addr(const char *object_name, const char *sym_name, unsigned long *addr) {

    struct r_debug *r_debug_ptr = &_r_debug;
    if (r_debug_ptr == NULL) {
        udi_printf("%s\n", "unable to locate r_debug ptr in current process");
        return -1;
    }

    struct link_map *cur_link = r_debug_ptr->r_map;
    while (cur_link != NULL) {
        size_t name_length = strlen(cur_link->l_name) + 1;
        char *name = (char *)udi_malloc(name_length);
        strncpy(name, cur_link->l_name, name_length);
        name[name_length-1] = '\0';

        char *obj_base = basename(name);

        if (strcmp(object_name, obj_base) == 0) {
            udi_free(name);
            return search_for_sym_in_obj(cur_link->l_name, sym_name, cur_link->l_addr, addr);
        }
        udi_free(name);
        cur_link = cur_link->l_next;
    }

    udi_printf("failed to determine address of symbol in %s(%s)\n", sym_name, object_name);
    return -1;
}

void (*pthreads_create_event)(void) = NULL;
void (*pthreads_death_event)(void) = NULL;

// start thread_db support

static
const char *THREAD_DB = "libthread_db.so";

// Define types for functions pulled from libthread_db
typedef td_err_e (*td_init_type)(void);
typedef td_err_e (*td_ta_new_type)(struct ps_prochandle *, td_thragent_t **);
typedef td_err_e (*td_ta_event_addr_type)(const td_thragent_t *,
        td_event_e, td_notify_t *);
typedef td_err_e (*td_ta_set_event_type)(const td_thragent_t *, td_thr_events_t *);
typedef td_err_e (*td_ta_map_id2thr_type)(const td_thragent_t *, pthread_t, td_thrhandle_t *);
typedef td_err_e (*td_thr_event_enable_type)(const td_thrhandle_t *, int);
typedef td_err_e (*td_thr_event_getmsg_type)(const td_thrhandle_t *, td_event_msg_t *);
typedef td_err_e (*td_ta_event_getmsg_type)(const td_thragent_t *, td_event_msg_t *);
typedef td_err_e (*td_thr_get_info_type)(const td_thrhandle_t *, td_thrinfo_t *);

struct ps_prochandle {
    // empty handle -- thread_db is loaded into every process
};

static
struct ps_prochandle ps_handle;

// thread_db members
td_thragent_t *thragent = NULL;
td_thr_event_getmsg_type td_thr_event_getmsg_func = NULL;
td_ta_event_getmsg_type td_ta_event_getmsg_func = NULL;
td_ta_map_id2thr_type td_ta_map_id2thr_func = NULL;
td_thr_get_info_type td_thr_get_info_func = NULL;
td_thr_event_enable_type td_thr_event_enable_func = NULL;

/**
 * Convenience function for calling dlsym
 */
static
void *local_dlsym(void *handle, const char *symbol, udi_errmsg *errmsg) {
    dlerror();
    void *result = dlsym(handle, symbol);
    char *dlerr_msg = dlerror();
    if (dlerr_msg != NULL) {
        snprintf(errmsg->msg, errmsg->size, "dlsym failed to locate %s: %s", 
                symbol, dlerr_msg);
        udi_printf("%s\n", errmsg->msg);
        return NULL;
    }

    return result;
}

/**
 * Convenience function for calling td_thr_event_enable
 */
static
int enable_thread_events(const td_thrhandle_t *handle, udi_errmsg *errmsg) {

    td_err_e td_ret;

    int thr_events[] = { TD_CREATE, TD_DEATH };
    for (int i = 0; i < (sizeof(thr_events)/sizeof(thr_events[0])); ++i) {
        if ( (td_ret = td_thr_event_enable_func(handle, thr_events[i])) != TD_OK ) {
            snprintf(errmsg->msg, errmsg->size, "td_thr_event_enable failed for event %d: %d",
                    thr_events[i], td_ret);
            udi_printf("%s\n", errmsg->msg);
            return -1;
        }
    }

    return 0;
}


/**
 * Initializes the newly created thread
 *
 * @param errmsg the error message (populated on error)
 *
 * @return the tid on success; 0 on failure
 */
uint64_t initialize_thread(udi_errmsg *errmsg) {

    td_err_e td_ret;

    td_event_msg_t event_msg;
    if ( (td_ret = td_ta_event_getmsg_func(thragent, &event_msg)) != TD_OK ) {
        snprintf(errmsg->msg, errmsg->size, "td_ta_event_getmsg failed: %d", td_ret);
        udi_printf("%s\n", errmsg->msg);
        return 0;
    }

    const td_thrhandle_t *new_handle = event_msg.th_p;
    if ( enable_thread_events(new_handle, errmsg) != 0 ) {
        return 0;
    }

    td_thrinfo_t new_info;
    if ( (td_ret = td_thr_get_info_func(new_handle, &new_info)) != TD_OK ) {
        snprintf(errmsg->msg, errmsg->size, "td_thr_get_info failed: %d", td_ret);
        udi_printf("%s\n", errmsg->msg);
        return 0;
    }

    return (uint64_t)new_info.ti_tid;
}

/**
 * Determines the thread that is in the process of being finalized
 *
 * @param errmsg the error message
 *
 * @return the tid for the finalized thread, 0 on error
 */   
uint64_t finalize_thread(udi_errmsg *errmsg) {

    /* for some reason, libthread_db is not returning consistent results for td_thr_event_getmsg
    td_err_e td_ret; 
    td_thrhandle_t dead_handle;
    if ( (td_ret = td_ta_map_id2thr_func(thragent, get_user_thread_id(), &dead_handle))
            != TD_OK )
    {
        snprintf(errmsg, errmsg_size, "td_ta_map_id2thr failed: %d", td_ret);
        udi_printf("%s\n", errmsg);
        return 0;
    }

    while (!value) {
        sleep(1);
    }

    td_event_msg_t event_msg;
    if ( (td_ret = td_thr_event_getmsg_func(&dead_handle, &event_msg)) != TD_OK ) {
        snprintf(errmsg, errmsg_size, "td_thr_event_getmsg failed: %d", td_ret);
        udi_printf("%s\n", errmsg);
        return 0;
    }

    if ( event_msg.event != TD_DEATH ) {
        snprintf(errmsg, errmsg_size, "unexpected event type: %d", event_msg.event);
        udi_printf("%s\n", errmsg);
        return 0;
    }

    const td_thrhandle_t *actual_handle = event_msg.th_p;
    td_thrinfo_t dead_info;
    if ( (td_ret = td_thr_get_info_func(actual_handle, &dead_info)) != TD_OK ) {
        snprintf(errmsg, errmsg_size, "td_thr_get_info failed: %d", td_ret);
        udi_printf("%s\n", errmsg);
        return 0;
    }

    return (uint64_t)dead_info.ti_tid;
    */
    return get_user_thread_id();
}

/**
 * Initializes pthreads support
 *
 * @param errmsg the errmsg populated on error
 *
 * @return 0 on success; non-zero on failure
 */
int initialize_pthreads_support(udi_errmsg *errmsg) {
    // Need to load and initialize thread_db
    if (get_multithread_capable()) {
        char *dlerr_msg;
        td_err_e td_ret;

        void *tdb_handle = dlopen(THREAD_DB, RTLD_NOW | RTLD_LOCAL);
        if (tdb_handle == NULL) {
            dlerr_msg = dlerror();
            snprintf(errmsg->msg, errmsg->size, "dlopen failed: %s", dlerr_msg);
            udi_printf("%s\n", errmsg->msg);
            return -1;
        }

        // get the relevant functions in thread_db
        td_init_type td_init_func = local_dlsym(tdb_handle, "td_init", errmsg);
        if (td_init_func == NULL) return -1;

        td_ta_new_type td_ta_new_func = local_dlsym(tdb_handle, "td_ta_new", errmsg);
        if (td_ta_new_func == NULL) return -1;

        td_ta_event_addr_type td_ta_event_addr_func = local_dlsym(tdb_handle,
                "td_ta_event_addr", errmsg);
        if (td_ta_event_addr_func == NULL) return -1;

        td_ta_set_event_type td_ta_set_event_func = local_dlsym(tdb_handle,
                "td_ta_set_event", errmsg);
        if (td_ta_set_event_func == NULL) return -1;

        td_ta_map_id2thr_func = local_dlsym(tdb_handle,
                "td_ta_map_id2thr", errmsg);
        if (td_ta_map_id2thr_func == NULL) return -1;

        td_thr_event_enable_func = local_dlsym(tdb_handle,
                "td_thr_event_enable", errmsg);
        if (td_thr_event_enable_func == NULL) return -1;

        td_thr_event_getmsg_func = local_dlsym(tdb_handle,
                "td_thr_event_getmsg", errmsg);
        if (td_thr_event_getmsg_func == NULL) return -1;

        td_ta_event_getmsg_func = local_dlsym(tdb_handle,
                "td_ta_event_getmsg", errmsg);
        if (td_ta_event_getmsg_func == NULL) return -1;

        td_thr_get_info_func = local_dlsym(tdb_handle,
                "td_thr_get_info", errmsg);
        if (td_thr_get_info_func == NULL) return -1;

        // initialize td_init
        if ( (td_ret = td_init_func()) != TD_OK ) { 
            snprintf(errmsg->msg, errmsg->size, "td_init failed: %d", td_ret);
            udi_printf("%s\n", errmsg->msg);
            return -1;
        }

        // get the thread agent handle
        if ( (td_ret = td_ta_new_func(&ps_handle, &thragent)) != TD_OK ) {
            snprintf(errmsg->msg, errmsg->size, "td_ta_new failed: %d", td_ret);
            udi_printf("%s\n", errmsg->msg);
            return -1;
        }

        // enable events globally
        td_thr_events_t events;
        td_event_emptyset(&events);
        td_event_addset(&events, TD_DEATH);
        td_event_addset(&events, TD_CREATE);
        if ( (td_ret = td_ta_set_event_func(thragent, &events)) != TD_OK ) {
            snprintf(errmsg->msg, errmsg->size, "td_ta_set_event failed: %d", td_ret);
            udi_printf("%s\n", errmsg->msg);
            return -1;
        }

        // enable events for the initial thread
        td_thrhandle_t initial_thread;
        if ( (td_ret = td_ta_map_id2thr_func(thragent, get_user_thread_id(), &initial_thread))
                != TD_OK )
        {
            snprintf(errmsg->msg, errmsg->size, "td_ta_map_id2thr failed: %d", td_ret);
            udi_printf("%s\n", errmsg->msg);
            return -1;
        }

        // enable all the interesting events
        if ( enable_thread_events(&initial_thread, errmsg) != 0 ) {
            return -1;
        }

        // get the addresses for the breakpoints
        td_notify_t notify_create;
        if ( (td_ret = td_ta_event_addr_func(thragent, TD_CREATE, &notify_create)) != TD_OK ) {
            snprintf(errmsg->msg, errmsg->size, "td_ta_event_addr failed: %d", td_ret);
            udi_printf("%s\n", errmsg->msg);
            return -1;
        }

        switch (notify_create.type) {
            case NOTIFY_BPT:
                break;
            case NOTIFY_AUTOBPT:
            case NOTIFY_SYSCALL:
                snprintf(errmsg->msg, errmsg->size, "notify type %d(0x%d) for create event not supported", 
                        notify_create.type, notify_create.u.syscallno);
                udi_printf("%s\n", errmsg->msg);
                return -1;
        }

        pthreads_create_event = (void(*)(void))notify_create.u.bptaddr;

        td_notify_t notify_death;
        if ( (td_ret = td_ta_event_addr_func(thragent, TD_DEATH, &notify_death)) != TD_OK ) {
            snprintf(errmsg->msg, errmsg->size, "td_ta_event_addr failed: %d", td_ret);
            udi_printf("%s\n", errmsg->msg);
            return -1;
        }

        switch (notify_death.type) {
            case NOTIFY_BPT:
                break;
            case NOTIFY_SYSCALL:
            case NOTIFY_AUTOBPT:
                snprintf(errmsg->msg, errmsg->size, "notify type %d(0x%d) for death event not supported", 
                        notify_death.type, notify_death.u.syscallno);
                udi_printf("%s\n", errmsg->msg);
                return -1;
        }

        pthreads_death_event = (void(*)(void))notify_death.u.bptaddr;
    }
    
    return 0;
}

// proc service interface

// copied from the proc_service.h header to make this source portable
typedef enum {
    PS_OK,                /* Generic "call succeeded".  */
    PS_ERR,               /* Generic error. */
    PS_BADPID,            /* Bad process handle.  */
    PS_BADLID,            /* Bad LWP identifier.  */
    PS_BADADDR,           /* Bad address.  */
    PS_NOSYM,             /* Could not find given symbol.  */
    PS_NOFREGS            /* FPU register set not available for given LWP.  */
} ps_err_e;

static udi_errmsg ps_errmsg;

ps_err_e ps_pdread(struct ps_prochandle *handle, psaddr_t src, void *dst, size_t size) {
    ps_errmsg.size = ERRMSG_SIZE;
    ps_errmsg.msg[ERRMSG_SIZE-1] = '\0';

    int result = read_memory(dst, (const void *)src, size, &ps_errmsg);

    if (result != 0) {
        udi_printf("read_memory in ps_pdread failed: %s\n", ps_errmsg.msg);
    }

    return ( result == 0 ? PS_OK : PS_ERR );
}

ps_err_e ps_pdwrite(struct ps_prochandle *handle, psaddr_t dst, const void *src, size_t size) {
    ps_errmsg.size = ERRMSG_SIZE;
    ps_errmsg.msg[ERRMSG_SIZE-1] = '\0';

    int result = write_memory((void *)dst, src, size, &ps_errmsg);

    if (result != 0) {
        udi_printf("write_memory in ps_pdwrite failed: %s\n", ps_errmsg.msg);
    }

    return ( result == 0 ? PS_OK : PS_ERR );
}

ps_err_e ps_ptread(struct ps_prochandle *handle, psaddr_t src, void *dst, size_t size) {
    return ps_pdread(handle, src, dst, size);
}

ps_err_e ps_ptwrite(struct ps_prochandle *handle, psaddr_t dst, const void *src, size_t size) {
    return ps_pdwrite(handle, dst, src, size);
}

pid_t ps_getpid(struct ps_prochandle *handle) {
    return getpid();
}

ps_err_e ps_pglobal_lookup(struct ps_prochandle *handle, const char *object_name, 
        const char *sym_name, psaddr_t *sym_addr) {

    unsigned long result;
    if ( get_sym_addr(object_name, sym_name, &result) != 0 ) {
        return PS_ERR;
    }

    *sym_addr = (psaddr_t)result;

    return PS_OK;
}

// the following functions should not be called

ps_err_e ps_lgetregs(struct ps_prochandle *handle, lwpid_t lwp, prgregset_t regset) {
    udi_abort(__FILE__, __LINE__);

    return PS_ERR;
}

ps_err_e ps_lsetregs(struct ps_prochandle *handle, lwpid_t lwp, const prgregset_t regset) {
    udi_abort(__FILE__, __LINE__);

    return PS_ERR;
}
ps_err_e ps_lgetfpregs(struct ps_prochandle *handle, lwpid_t lwp, prfpregset_t *regset) {
    udi_abort(__FILE__, __LINE__);

    return PS_ERR;
}

ps_err_e ps_lsetfpregs(struct ps_prochandle *handle, lwpid_t lwp, const prfpregset_t *regset) {
    udi_abort(__FILE__, __LINE__);

    return PS_ERR;
}

ps_err_e ps_get_thread_area(const struct ps_prochandle *handle, lwpid_t lwp, 
        int platform_specifc, psaddr_t *dst) {
    udi_abort(__FILE__, __LINE__);

    return PS_ERR;
}

ps_err_e ps_pstop(const struct ps_prochandle *handle) {
    udi_abort(__FILE__, __LINE__);

    return PS_ERR;
}

ps_err_e ps_pcontinue(const struct ps_prochandle *handle) {
    udi_abort(__FILE__, __LINE__);

    return PS_ERR;
}

ps_err_e ps_lstop(const struct ps_prochandle *handle, lwpid_t lwp) {
    udi_abort(__FILE__, __LINE__);

    return PS_ERR;
}

ps_err_e ps_lcontinue(const struct ps_prochandle *handle, lwpid_t lwp) {
    udi_abort(__FILE__, __LINE__);

    return PS_ERR;
}

// end thread_db support

/**
 * @return the kernel thread id for the currently executing thread
 */
uint32_t get_kernel_thread_id() {
    return (uint32_t)syscall(SYS_gettid);
}
