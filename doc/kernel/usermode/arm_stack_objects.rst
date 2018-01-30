.. _arm_stack_objects:

ARM Stack Objects
#################

Thread Stack Creation
=====================

Thread stacks are declared staticly with :c:macro:`K_THREAD_STACK_DEFINE()`
or embedded within structures using c:macro:`K_THREAD_STACK_MEMBER()`

Due to the ARM architecture not including an MMU, stacks are physically
contiguous allocations.  This contiguous allocation has implications
for the placement of stacks in memory, as well as the implementation of
other features such as stack protection and userspace.

Stack Guards
============

Stack protection mechanisms require hardware support.  In the case of ARM
architectures, this is achieved through the use of a memory protection unit.
The MPU provides a fixed number of regions.  Each region contains information
about the start, end, size, and access attributes to be enforced on that
particuliar region.

Stack guards are implemented by using a single MPU region and setting the
attributes for that region to not allow write access.  If invalid accesses
occur, a fault ensues.  The stack guard is defined at the bottom of the stack,
or the lowest address.

Memory Placement
================

During stack creation, a set of constraints are enforced on the allocation of
memory.  These contraints include determining the alignment of the stack and
the correct sizing of the stack.  During linking of the binary, these
constraints are used to place the stacks properly.

The main source of the memory constraints is the MPU design for the ARM SoC.
Some MPUs require that each region be aligned to a power of two.  These SoCs
will have :option:`CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT` defined.
This means that a 1500 byte stack should be aligned to a 2kB boundary.  This
requires a power of two ceiling to be done on the alignment and size of the
stack.  The result is that the actual memory allocation of a given stack may
be larger than the request.

For MPUs which do not have a power of two alignment constraint, the minimum
alignment is required to be 32 bytes.

Userspace
=========

The ARM userspace implementation requires the creation of a secondary set of
stacks.  These stacks exist in a 1:1 relationship with each thread stack
defined in the system.  The privileged stacks are created as a part of the
build process.

A post-build script ``gen_priv_stacks.py`` scans the generated
ELF file and finds all of the thread stack objects.  A set of priviliged
stacks, a lookup table, and a set of helper functions are created and added
to the image.

During the process of dropping a thread to user mode, the privileged stack
information is filled in and later used by the swap and system call
infrastructure to configure the MPU regions properly for the thread stack and
guard (if applicable).

During system calls, the user mode thread's access to the system call and the
passed in parameters are all validated.  The user mode thread is then elevated
to privileged mode, the stack is switched to use the privileged stack, and the
call is made to the specified kernel API.  On return from the kernel API,  the
thread is set back to user mode and the stack is restored to the user stack.

