Project:    Phase2 
File:       README.txt
Authors:    Kiera   Conway
            Katelyn Griffith

Contents:
	
	testcases:		Since modifications were made to the
					passed in C libraries (ie changed
					strings.h to string.h), we included
					the cases in their entirety. There 
					were no code modifications. 
	
	makefile:		Automates compilation and linking process
	
	handler.h:		Created to fix linking issues between
					phase2.c and handler.c
	handler.c:		Contains the interrupt handler functions
	
	lists.h
	lists.c:		Holds the functions and globals for the 
					linked lists used
		
	message.h:		Contains the constants, typedefs, 
					structures, and external function prototypes 
					used throughout the program.
				
	phase2.c:		Contains the required functions necessary to 
					send and receive messages. Must be used with 
					lists.h/.c and tables.h/.c to pass test00-42.
	
	README.txt:		General information regarding project
	
	tables.h
	tables.c:		Contains the functions and globals for the 
					MailboxTable and SlotTable. Also contains
					the functions necessary for mboxReleaser
					to work.
					
	testOutput.txt:	Output compilation for all test cases
	

Notes:
   
	-	USLOSS not included
			
	-	A few of our functions are 'general purpose' and work 
		on multiple structure types. However, in order to do this
		we had to pass NULL in some of our function calls. The 
		warnings that appear on compilation are a result of this.
		Please note that this is intentional - We have all the 
		necessary checks in place, the code compiles, and the 
		test cases pass.
		
	-	Phase 2 required functions to initialize, update, 
		remove,	and check linked lists and arrays so we moved
		those functions to lists.c and tables.c. A list of these
		external functions are below.
		

Overview of Functions:
	Tables.c:
		Functions for MailboxTable:
			GetNextMailID()		-	Gets the next available MailboxTable 
									slot.
			InitializeMailbox() - 	This functions handles two things: 
									(1) Initializing a MailboxTable slot 
									(2) Removing a mailbox from the MailboxTable.
			GetMboxIndex()      - 	This returns the mailbox position with the 
									MailboxTable based on the mailboxID.
			HelperRelease()     - 	Handles unblocking to be released processes.
			MboxWasReleased()   - 	Checks if a mailbox was released and handles 
									unblocking a releaser process.

		Functions for SlotTable:
			GetNextSlotID()     - 	Get the next available SlotTable slot.
			InitializeSlot()    - 	This functions handles two things: 
									(1) Initializing a SlotTable slot and 
									(2) Removing a slot from the SlotTable.
			GetSlotIndex()      - 	This returns the slot position with the 
									SlotTable based on the slotID.
		
	Lists.c:
		Shared Message and Process Functions:
			InitializeList()    - 	Initializes the process and slot linked lists 
									for a mailbox.
			ListIsFull()        - 	Checks if a message or process list is full.
			ListIsEmpty()       - 	Checks if a message list is full.
			
		Functions for Messages:
			AddSlotLL()         - 	Adds a message to the message list.
			RemoveSlotLL()      - 	Removed the head message from the message list.
			FindSlotLL()        - 	Finds a message within the message list.
			
		Functions for Processes:
			AddProcessLL()      - 	Adds a process to the process list.
			RemoveProcessLL()   - 	Removed the head process from the process list.
			CopyProcessLL()     - 	Copies the values of a process to another process.
			FindProcessLL()     - 	Finds a process within the process list.