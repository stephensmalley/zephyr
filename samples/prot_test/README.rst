.. _prot_test:

Protection tests
#################################

Overview
********
This test provides a set of shell commands to test
protection against the following security issues:

* Write to read-only data.
* Write to text.
* Execute from data.
* Execute from stack.
* Execute from heap.

Messages that begin with FAIL: are indicative of a lack of
suitable protection.

Building and Running
********************

This project can be built and executed as follows:

.. code-block:: console

   $ cd samples/prot_test
   $ make BOARD=<insert your board here>

Connect the board to your host computer using the USB port.
Flash the generated zephyr.bin on the board.
Reset the board and you should be able to see on the corresponding
serial port the following message:

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.7.99 - BUILD: Mar 20 2017 14:52:12 *****
   shell>
   prot_test>

Sample Output
=============

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 13 2017 15:43:19 *****
   shell>
   prot_test> help
   help
   write_ro
   write_text
   exec_data
   exec_stack
   exec_heap
   disable_mpu
   enable_mpu

   Enter 'exit' to leave the current module.
   prot_test> disable_mpu
   prot_test> exec_stack
   trying to call code written to 0x20000ee9
   returned from code at 0x20000ee9
   FAIL: Execute from target buffer succeeded!
   prot_test> enable_mpu
   prot_test> exec_stack
   ***** BUS FAULT *****
   Executing thread ID (thread): 0x20000400
   Faulting instruction address:  0x20000ee8
   Instruction bus error
   Fatal fault in thread 0x20000400! Aborting.
