#include "BTreeNode.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <math.h>
// Remove for loop from readEntry

using namespace std;

BTLeafNode::BTLeafNode()
{
std::fill(buffer, buffer + PageFile::PAGE_SIZE, 0); //init buffer
}

void BTLeafNode::print()
{
	int key;
	RecordId rid;
	char * p = buffer;
	int rec = 0;
	memcpy(&rec, p, sizeof(int));
	for(int i=1; i<=rec; i++)
	{
	readEntry(i, key, rid);
	}
	PageId nextPtr;
	memcpy(&nextPtr, p+(rec*LEAF_ENTRY_SIZE)+sizeof(int), PAGE_ID_SIZE);
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{ 	
return pf.read(pid,buffer);
   
} 
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{ 	
	return pf.write(pid, buffer);
		
}
	

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{	//Get pointer to the buffer associated with the node,which was populated when node was read.
	char *p = buffer;
	int i;
  	int no_of_records = 0;
  	memcpy(&no_of_records, p, sizeof(int));
  	return no_of_records;
  	/*int temp_key;
  	//fprintf(stdout, "Entry size is: %d\n",LEAF_ENTRY_SIZE);
  	for(i = RECORD_ID_SIZE;p[i];i+=LEAF_ENTRY_SIZE) {
  		records = records+1;
  	}
  	p = p+RECORD_ID_SIZE;
  	while(true) {
  		memcpy(&temp_key, p, KEY_SIZE);
  		if(temp_key == 0){
  			break;
  		}
  		no_of_records+=1;
  		p = p+LEAF_ENTRY_SIZE;

  	}
  	return no_of_records;*/
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{ 	int no_of_records = getKeyCount();
	if(no_of_records >= MAX_KEYS) {
		return RC_NODE_FULL;
	}
	if(rid.pid<0 || rid.sid<0 || rid.sid > RecordFile::RECORDS_PER_PAGE){
		return RC_INVALID_RID;
	}
	int eid;
	int curr_key;
	RecordId curr_rid;
	for(eid=1; eid<=no_of_records; eid++) {
		readEntry(eid,curr_key,curr_rid);
		if(curr_key<key){
			//continue for loop
		} else {
			break;
		}

	}
	char *p = buffer;

	//Update the key Count in node.
	no_of_records = no_of_records + 1;
	memcpy(p, &no_of_records, sizeof(int));

  	//Move to start of record which needs to be shifted right(Also leave 4 bytes of #keys)
  	p = p+((eid-1)*LEAF_ENTRY_SIZE)+sizeof(int);
  	//Shifting all entries and next node pointer to the right by 12 bytes 
  	memmove(p+LEAF_ENTRY_SIZE,p,((no_of_records-(eid-1))*LEAF_ENTRY_SIZE)+PAGE_ID_SIZE);
  	memcpy(p,&rid,RECORD_ID_SIZE);
	memcpy(p+RECORD_ID_SIZE,&key,KEY_SIZE);
	//print();
	//fprintf(stdout, " SUCCESSFUL INSERT IN LEAF NODE AND KEY COUNT IS NOW %d \n", getKeyCount());
	return 0;
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{ 	
	int eid;
	int curr_key;
	RecordId curr_rid;
	for(eid=1; eid<=MAX_KEYS; eid++) {
		readEntry(eid, curr_key, curr_rid);
		if(curr_key>key){
			break;
		} 
	}

	char *p = buffer;
	p = (p+((eid-1)*LEAF_ENTRY_SIZE)+sizeof(int));

	memmove(p+LEAF_ENTRY_SIZE,p,((MAX_KEYS-(eid-1))*LEAF_ENTRY_SIZE)+PAGE_ID_SIZE);
	memcpy(p,&rid,RECORD_ID_SIZE);
	memcpy(p+RECORD_ID_SIZE,&key,KEY_SIZE);

	//moving into sibling buffer
	int midIndex = (MAX_KEYS+1)/2;
	int currNoKeys = midIndex;
	int siblingNoKeys = (MAX_KEYS/2) + 1;

	p = buffer;
	//init 1st 4 bytes to no of keys
	memmove(sibling.buffer, &siblingNoKeys, KEY_SIZE);
	memmove(sibling.buffer + KEY_SIZE, p + KEY_SIZE + (midIndex*LEAF_ENTRY_SIZE), PageFile::PAGE_SIZE - (midIndex*LEAF_ENTRY_SIZE) + KEY_SIZE);
	//sibling.print();
	std::fill(buffer + 4 + (midIndex*LEAF_ENTRY_SIZE), buffer + PageFile::PAGE_SIZE , 0);
	//init 1st 4 bytes to no of keys
	memmove(buffer, &currNoKeys, KEY_SIZE);
	//print();
	memcpy(&siblingKey, sibling.buffer  + KEY_SIZE + RECORD_ID_SIZE, KEY_SIZE);
	
	return 0;
}

/**
 * If searchKey exists in the node, set eid to the index entry
 * with searchKey and return 0. If not, set eid to the index entry
 * immediately after the largest index key that is smaller than searchKey,
 * and return the error code RC_NO_SUCH_RECORD.
 * Remember that keys inside a B+tree node are always kept sorted.
 * @param searchKey[IN] the key to search for.
 * @param eid[OUT] the index entry number with searchKey or immediately
                   behind the largest key smaller than searchKey.
 * @return 0 if searchKey is found. Otherwise return an error code.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{ 	int no_of_records = getKeyCount();
	int curr_key;
	int curr_eid;
	RecordId curr_rid;
	for(curr_eid=1;curr_eid<=no_of_records;curr_eid+=1) {
		readEntry(curr_eid,curr_key,curr_rid);
		//fprintf(stdout, "READ ENTRY NO: %d %d %d:%d\n",curr_eid,curr_key,curr_rid.pid,curr_rid.sid);
		if(searchKey == curr_key) {
			eid = curr_eid;
			return 0;
		}	
		if(curr_key>searchKey) {
			//eid = curr_eid;
			break;
		}
		eid = curr_eid;
	}
	return RC_NO_SUCH_RECORD;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{ 	RC rc;
	char *p = buffer;
	int i;
	if(eid <= 0 || eid>getKeyCount()) {
		return RC_NO_SUCH_RECORD;
	}
  	int current_eid = 0;

  	//Need to offset p by 4 bytes(of #keys also)
  	p = p+((eid-1)*LEAF_ENTRY_SIZE)+sizeof(int);
  
  	memcpy(&rid,p, RECORD_ID_SIZE);
  	memcpy(&key,p+RECORD_ID_SIZE,KEY_SIZE);
  	return 0; 
  	
}
/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{ 	char *p = buffer;

  	//PageId pid =*(p+(getKeyCount()*entry_size)+key_size);
  	//Return pid as -1 if sibling doesnt exist.
  	PageId nextPtr = -1;
	memcpy(&nextPtr, p+(getKeyCount()*LEAF_ENTRY_SIZE)+sizeof(int),PAGE_ID_SIZE);
	return nextPtr;
 }

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{ if(pid<0) {
	return RC_INVALID_PID;
  }

  //int no_of_records = getKeyCount();
  
  char *p = buffer;
  memcpy(p+(getKeyCount()*LEAF_ENTRY_SIZE)+sizeof(int), &pid, PAGE_ID_SIZE);
  //*(p+(no_of_records*entry_size)) = pid;
  return 0; 
}

BTNonLeafNode::BTNonLeafNode()
{
std::fill(buffer, buffer + PageFile::PAGE_SIZE, 0); //init buffer
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */

void BTNonLeafNode::print()
{int key;
	PageId pid;
	/*
	for(int i=1; i<= getKeyCount(); i++)
	{
	readEntryNonLeaf(i, key, pid);
		
	}
	*/
	
	
}
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{ 

   return pf.read(pid,buffer);
   
}

RC BTNonLeafNode::readEntryNonLeaf(int eid, int& key, PageId& pid)
{ 	
	if(eid<0){
		return RC_NO_SUCH_RECORD;
	}
	char *p = buffer;
	int i;
 	
  	p = p+4+sizeof(int)+((eid-1)*NON_LEAF_ENTRY_SIZE);

  	memcpy(&key, p, KEY_SIZE);
  	memcpy(&pid, p+KEY_SIZE, PAGE_ID_SIZE);

  	return 0; 
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{ 
	
	return pf.write(pid, buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{   char *p = buffer;
  	int no_of_records = 0;
	memcpy(&no_of_records, p, sizeof(int));  
	return no_of_records;
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{ 	int no_of_records = getKeyCount();
	if(no_of_records >= MAX_KEYS) {
		return RC_NODE_FULL;
	}
	if(pid<0){
		return RC_INVALID_PID;
	}
	int eid;
	int curr_key;
	PageId curr_pid;
	for(eid=1;eid<=no_of_records;eid++) {
		readEntryNonLeaf(eid,curr_key,curr_pid);
		if(curr_key<key){
			//continue for loop
		} else {
			break;
		}

	}
	char *p = buffer;

	//Update the key Count in node.
	no_of_records = no_of_records + 1;
	memcpy(p, &no_of_records, sizeof(int));
	
	//Offseting p by 4 bytes for PID1:
	//Offsetting by int size for key count stored in 1 st 4 bytes
  	p = p+4+sizeof(int)+((eid-1)*NON_LEAF_ENTRY_SIZE); 
  	memmove(p+NON_LEAF_ENTRY_SIZE, p, ((no_of_records-(eid-1))*(NON_LEAF_ENTRY_SIZE))); 
	memcpy(p, &key, KEY_SIZE);
	memcpy(p+KEY_SIZE, &pid, PAGE_ID_SIZE);

	return 0; 
}


/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{
	int eid;
	int curr_key;
	PageId curr_pid;
	for(eid=1; eid<=MAX_KEYS; eid++) {
		readEntryNonLeaf(eid, curr_key, curr_pid);
		if(curr_key>key){
			break;
		} 
	}

	char *p = buffer;
	p = (p+((eid-1)*NON_LEAF_ENTRY_SIZE)+sizeof(int) + PAGE_ID_SIZE);

	memmove(p+NON_LEAF_ENTRY_SIZE, p, ((MAX_KEYS-(eid-1))*NON_LEAF_ENTRY_SIZE)+PAGE_ID_SIZE);
	memcpy(p, &key, KEY_SIZE);
	memcpy(p + KEY_SIZE, &pid, PAGE_ID_SIZE);

	//moving into sibling buffer
	int midIndex = (MAX_KEYS)/2 + 1;
	int currNoKeys = MAX_KEYS/2;
	int siblingNoKeys = (MAX_KEYS+1)/2;

	p = buffer;
	//init 1st 4 bytes to no of keys
	memmove(sibling.buffer, &siblingNoKeys, KEY_SIZE);
	PageId midPid;
	readEntryNonLeaf(midIndex, midKey, midPid);
	//setting left ptr
	memmove(sibling.buffer + KEY_SIZE, &midPid, KEY_SIZE);
	memmove(sibling.buffer + (2*KEY_SIZE), p + (2*KEY_SIZE) + (midIndex*NON_LEAF_ENTRY_SIZE), PageFile::PAGE_SIZE - (midIndex*NON_LEAF_ENTRY_SIZE) + (2*KEY_SIZE));

	//sibling.print();
	std::fill(buffer + (2*KEY_SIZE) + ((midIndex-1)*NON_LEAF_ENTRY_SIZE), buffer + PageFile::PAGE_SIZE , 0);
	//init 1st 4 bytes to no of keys
	memmove(buffer, &currNoKeys, KEY_SIZE);
	//print();
	
	
	return 0;

}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{ 	RC rc;
	int no_of_records = getKeyCount();
	char *p = buffer;
	int curr_key;
	int curr_eid;
	PageId curr_pid;
	

	for(curr_eid=1;curr_eid<=no_of_records;curr_eid+=1) {
		if(rc=readEntryNonLeaf(curr_eid,curr_key,curr_pid)<0) {
			return rc;
		}
		if(searchKey < curr_key) {
			memcpy(&pid,p+((curr_eid-1)* NON_LEAF_ENTRY_SIZE)+sizeof(int), PAGE_ID_SIZE);
			return 0;
		}
		
	}
	//Search Key is greater greater all keys in Non Leaf Node
	memcpy(&pid,p+((curr_eid-1)* NON_LEAF_ENTRY_SIZE)+sizeof(int), PAGE_ID_SIZE);
	return 0;
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{ 	RC rc;
	if(pid1<0 || pid2<0) {
		return RC_INVALID_PID;
	}

	char *p = buffer;

	//Setting the leftmost pointer once for new node
	memcpy(p+sizeof(int), &pid1, PAGE_ID_SIZE);


	if(rc=insert(key,pid2) < 0){
		return rc;
	}
	return 0; 
}
