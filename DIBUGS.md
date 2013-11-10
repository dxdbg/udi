This file is an attempt to create a list illustrating the prevalence and 
complexity of bugs in kernel provided debugger interfaces.

FreeBSD
-------

* privilege escalation via mmap and ptrace interaction

http://www.freebsd.org/security/advisories/FreeBSD-SA-13:06.mmap.asc

Linux
-----

* privilege escalation by writing shellcode into /proc/pid/mem

http://blog.zx2c4.com/749
