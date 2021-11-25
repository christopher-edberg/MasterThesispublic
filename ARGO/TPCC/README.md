# Debugging

	There are 3 modes of debugging implemented, these can be set by the TPCC_DEBUG parameter in tpcc_db.h.
		TPCC_DEBUG = 0 (disabled)
		TPCC_DEBUG = 1 
		TPCC_DEBUG = 2
		TPCC_DEBUG = 3
## TPCC_DEBUG = 1

	This mode will print various information about function calls and variable values at certain points in the program.
	See lines: 383, 403, 606, 637, 651, 663, 732 (lines are approximate pending future code changes) 

## TPCC_DEBUG = 2

	Same as previous mode with the exception that it prints the information to the file out.txt instead of to the terminal.


## TPCC_DEBUG = 3

	This mode is concentrated on the critical section in new_order_tx and will print information of the function calls and the values of the variables being passed in said calls. This mode is for verifying that the critical sections are executed correctly. 
		
	It will also print before and after values of the relevant variables being altered inside the functions that are being called.

	See files: fill_new_order_entry.txt,  fill_new_order_entry_change.txt,
		   update_order_entry.txt, update_order_entry_change.txt,
		   update_stock_entry.txt, update_stock_entry_change.txt

## TPCC_DEBUG = 4

	Automated debug mode, will calculate values of variables to see if they were updated appropriately.
	
	This mode specifically checks if coherence was upheld. 
