Project:    Phase3 
File:       README.txt
Authors:    Kiera   Conway
            Katelyn Griffith

Contents:
	
	testcases:			Used to test the functionality of Phase3 - 
						there were no modifications to code. 
	
	makefile:			Automates compilation and linking process
		
	lists.h
	lists.c:			Contains functions and globals for the 
						linked lists
					
	tables.h
	tables.c:			Contains the functions and globals for the 
						UserProcTable and SemaphoreTable 
	
	sems.h:				Contains globals, external function prototypes 
						structures, and typedefs for semaphores
						
						
	phase3_helper.h:	Contains globals, external function prototypes 
						structures, and typedefs for processes

	phase3.c:			Contains the required functions necessary to handle 
						user processes and system calls. Must be used with 
						lists.h/.c and tables.h/.c to pass test00-25.
	
	README.txt:			General information regarding project
					
	testOutput.txt:		Output compilation for all test cases
	

Notes:
   
	-	USLOSS not included
	
	-	Phase 3 required functions to initialize, update, 
		remove,	and check linked lists and arrays so we moved
		those functions to lists.c and tables.c. A list of these
		external functions are below.
		

Overview of Functions:
	Tables.c:
		Functions for Processes:

			ProcessInit() 	 	This functions handles two things: 
									(1) Initializing a UsrProcTable slot to -1
									(2) Removing a process from the UsrProcTable.
			
			AddToProcTable()	Adds a new process to the UsrProcTable and updates 
								the status to newStatus
								
			GetProcIndex		Returns the process position inside UsrProcTable 
								based on the passed in PID						
			
		Functions for Semaphores:	
			
			SemaphoreInit()		This functions handles two things: 
									(1) Initializing a SemTable slot to -1
									(2) Removing a semaphore from the SemTable.
			
			AddToSemTable()		Adds a new semaphore to the SemTable and updates 
								the status to newStatus
								
			GetSemIndex			Returns the semaphore position inside SemTable 
								based on the passed in SID

	Lists.c:
		Functions for Processes:
			ListIsFull()        - 	Checks if a process or sem list is full.
			ListIsEmpty()       - 	Checks if a process or sem list is empty.
			InitializeList()    - 	Initializes the process list.
			AddProcessLL()      - 	Adds a process to the process list.
			RemoveProcessLL()   - 	Removed the head process from the process list.