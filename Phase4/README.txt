Project:    Phase4 
File:       README.txt
Authors:    Kiera   Conway
            Katelyn Griffith

Contents:
	
	testcases:			Used to test the functionality of Phase4 - 
						there were no modifications to code. 
	
	makefile:			Automates compilation and linking process.
						
						
	process.h:			Contains globals and typedefs for processes.

	phase4.c:			Contains the required functions necessary to handle
						the clock driver, disk driver, sleeping processes,
						read/writing to a disk, and getting the size of 
						a disk. Must be used with process.h to pass test00-13.
	
	README.txt:			General information regarding project
					
	testOutput.txt:		Output compilation for all test cases
	

Notes:
   
	-	USLOSS not included
	
	-	Phase 4 required functions to initialize, update,
		remove, and check linked lists and tables.
		They are included within phase4.c.
	
	-	Our phase 4 implements the FCFS algorithm.
		

Overview of Functions for Lists and Tables:
	Table Functions:
	
		GetIndex()			-	Returns the process position inside ProcTable 
								based on the passed in PID
		AddToProcTable()	-	Adds a new process to the ProcTable
		CreatePointer()		-	Creates a pointer to a specific process within the ProcTable			

	List Functions:
	
		InitializeList()    - 	Initializes a process list
		ListIsFull()        - 	Checks if a process list is full
		ListIsEmpty()       - 	Checks if a process list is empty
		AddToList()         - 	Adds a process to a process list
		RemoveFromList()    - 	Removed the head process from a process list
