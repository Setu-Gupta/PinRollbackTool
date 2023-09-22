# PinRollbackTool

This tool demonstrates the use of the `PIN_ExecuteAt()` API. This tools forces the program execution on the wrong branch paths and the rolls back the state to the correct branch path. Note that PIN can only rollback register state and *NOT* the memory state. This means that if a memory store occurs on the wrong path, then that store will persist even after the execution is redirected on the correct path.

## Testing Environment
Pin verison: 3.28-98749-g6643ecee5-gcc-linux

CPU: Intel(R) Core(TM) i7-4770 CPU

## How to build
* Copy this directory to `<pin installation>/source/tools`
* Go to the pintool directory
	```
	$ cd <pin installation>/source/tools/PinRollbackTool 
	```
* Build the pintool
	$ make

## How to clean
	$ make clean

## How to run
* Go to the pin installation directory
	```	
	$ cd <pin installation>
	```
* Execute Pin with PinRollbackTool
	```
	$ ./pin -t source/tools/PinRollbackTool/obj-intel64/PinRollbackTool.so -n <number of speculatively executed instructions after which rollback should be triggered> -- <path to the program>
	```
