Project:    Phase2 
File:       README.txt
Authors:    Kiera   Conway
            Katelyn Griffith
     
Notes:
   
	-	Phase 2 used a lot of functions to initialize/update/remove/check linked lists and arrays so we moved those functions to lists.c and tables.c.
	-	Tables.c holds the functions and globals for the MailboxTable and SlotTable. 
	-	Lists.c holds the functions and globals for the linked lists used.
		
		Tables.c:
		Functions for MailboxTable:
			GetNextMailID()     - Gets the next available MailboxTable slot.
			InitializeMailbox() - This functions handles two things: (1) Initializing a MailboxTable slot and (2) Removing a mailbox from the MailboxTable.
			GetMboxIndex()      - This returns the mailbox position with the MailboxTable based on the mailboxID.
			HelperRelease()     - Handles unblocking to be released processes.
			MboxWasReleased()   - Checks if a mailbox was released and handles unblocking a releaser process.
			
		Functions for SlotTable:
			GetNextSlotID()     - Get the next available SlotTable slot.
			InitializeSlot()    - This functions handles two things: (1) Initializing a SlotTable slot and (2) Removing a slot from the SlotTable.
			GetSlotIndex()      - This returns the slot position with the SlotTable based on the slotID.
			
		Lists.c:
		Shared Message and Process Functions:
			InitializeList()    - Initializes the process and slot linked lists for a mailbox.
			ListIsFull()        - Checks if a message or process list is full.
			ListIsEmpty()       - Checks if a message list is full.
			
		Functions for Messages:
			AddSlotLL()         - Adds a message to the message list.
			RemoveSlotLL()      - Removed the head message from the message list.
			FindSlotLL()        - Finds a message within the message list.
			
		Functions for Processes:
			AddProcessLL()      - Adds a process to the process list.
			RemoveProcessLL()   - Removed the head process from the process list.
			CopyProcessLL()     - Copies the values of a process to another process.
			FindProcessLL()     - Finds a process within the process list.
		
		

