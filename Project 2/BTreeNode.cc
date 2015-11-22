#include "BTreeNode.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <math.h>
// Remove for loop from readEntry

using namespace std;

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{ 
	cout <<"Reading from page into buffer:\n";
	
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
	cout <<"Writing from buffer to page: "<<buffer;
	return pf.write(pid, buffer);
		
}
	

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{	//Get pointer to the buffer associated with the node,which was populated when node was read.
	char *p = buffer;
	cout << "Buffer while fetching Key Count is "<<buffer<<"\n";
	int i;
  	int records = 0;
  	fprintf(stdout, "Entry size is: %d\n",LEAF_ENTRY_SIZE);
  	for(i = RECORD_ID_SIZE;p[i];i+=LEAF_ENTRY_SIZE) {
  		cout <<"Val is"<< p[i];
  		records = records+1;
  	}
  	//cout <<"Current Key Count="<<records<<"\n";
  	return records;
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{ 	int no_of_records = getKeyCount();
	if(no_of_records >= 70) {
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
  	//Move to start of record which needs to be shifted right
  	p = p+((eid-1)*LEAF_ENTRY_SIZE);
  	memmove(p+LEAF_ENTRY_SIZE,p,(no_of_records-(eid-1))*LEAF_ENTRY_SIZE);
  	memcpy(p,&rid,RECORD_ID_SIZE);
	memcpy(p+RECORD_ID_SIZE,&key,KEY_SIZE);
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
{ 	RC rc;
	char *p = buffer;
	int no_of_records = getKeyCount();
	if(no_of_records >= 70 || sibling.getKeyCount() >=70) {
		return RC_NODE_FULL;
	}
	if(rid.pid<0 || rid.sid<0 || rid.sid > RecordFile::RECORDS_PER_PAGE){
		return RC_INVALID_RID;
	}

	int mid_record = ceil(no_of_records/2);
	char *next_p = sibling.buffer;
	
	memmove(next_p,p+(LEAF_ENTRY_SIZE*(mid_record)),(no_of_records-mid_record)*LEAF_ENTRY_SIZE);
	if(rc=sibling.insert(key,rid) < 0){
		return rc;
	}
	int prev = mid_record-1;
	int prev_key;
	RecordId prev_rid;
	readEntry(prev,prev_key,prev_rid);
	if(key<prev_key){
		insert(key,rid);
	} else {
		sibling.insert(key,rid);
	}

	//setNextNodePtr(sibling.PageId); from BTreeIndex??
	memcpy(&siblingKey,next_p+RECORD_ID_SIZE,KEY_SIZE);
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
		if(searchKey == curr_key) {
			eid = curr_eid;
			return 0;
		}	
		if(curr_key>searchKey) {
			eid = curr_eid;
			break;
		}
		return RC_NO_SUCH_RECORD;
	}
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
	cout<<"Inside Read entry and buffer is "<<buffer;
	int i;
	if(eid <= 0 || eid>getKeyCount()) {
		return RC_NO_SUCH_RECORD;
	}
  	int current_eid = 0;
  
  	memcpy(&rid,p+((eid-1)*LEAF_ENTRY_SIZE),RECORD_ID_SIZE);
  	p = p+((eid-1)*LEAF_ENTRY_SIZE);
  	memcpy(&key,p+RECORD_ID_SIZE,KEY_SIZE);
  	return 0; 
  	/*
  	//*p=123;
  	//*(p+4) = 58;
  	int a=123456;
  	int b=567;
  	int a1=0;int b1=0;
  	memcpy(p,&a,sizeof(int));
  	memcpy(p+sizeof(int),&b,sizeof(int));
  	//memcpy(buffer[4],&b,sizeof(int));
  	
  	memcpy(&a1,p,sizeof(int));
  	memcpy(&b1,p+sizeof(int),sizeof(int));
  	cout<<"first int=\n"<<p[0];
  	cout<<"next int=\n"<<p[3];
  	cout<<"a1="<<a1<<"\n";
  	cout<<"b1="<<b1<<"\n";
  	RecordId rr;
  	rr.pid = 12;
  	rr.sid = 34;
  	memcpy(p,&rr,8);
  	memcpy(&a1,p,sizeof(int));
  	memcpy(&b1,p+sizeof(int),sizeof(int));

  	cout<<"PageId="<<a1;
  	cout<<"SlotId="<<b1;
  	*/
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{ 	char *p = buffer;
	

  	//PageId pid =*(p+(getKeyCount()*entry_size)+key_size);
  	PageId pid;
  	memcpy(&pid, p+(getKeyCount()*LEAF_ENTRY_SIZE), PAGE_ID_SIZE);
  	return pid;
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

  int no_of_records = getKeyCount();
  
  char *p = buffer;
  memcpy(p+getKeyCount()*LEAF_ENTRY_SIZE,&pid,PAGE_ID_SIZE);
  //*(p+(no_of_records*entry_size)) = pid;
  return 0; 
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
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
  	int current_eid = 0;
  	

  	memcpy(&pid, p+((eid-1)*NON_LEAF_ENTRY_SIZE), PAGE_ID_SIZE);
  	p = p+((eid-1)*NON_LEAF_ENTRY_SIZE);
  	memcpy(&key, p+PAGE_ID_SIZE, KEY_SIZE);
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
	int i;
  	int records = 0;
  	
  	for(i = PAGE_ID_SIZE;p[i];i+=NON_LEAF_ENTRY_SIZE) {
  		records = records+1;
  	}
  	return records; 
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{ 	int no_of_records = getKeyCount();
	if(no_of_records > 70) {
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
	
  	p = p+((eid-1)*NON_LEAF_ENTRY_SIZE); 
  	memmove(p+NON_LEAF_ENTRY_SIZE,p,((no_of_records-eid-1)*(NON_LEAF_ENTRY_SIZE))); 
	memcpy(p,&pid,PAGE_ID_SIZE);
	memcpy(p+PAGE_ID_SIZE,&key,KEY_SIZE);
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
{ 	RC rc;
	char *p = buffer;
	int no_of_records = getKeyCount();
	int mid_record = ceil(no_of_records/2);
	int prev_record = mid_record - 1;
	char *next_p = sibling.buffer;
	//Find the mid key for the non-leaf node
	int mid_key,prev_key;
	PageId mid_pid,prev_pid;

	readEntryNonLeaf(mid_record, mid_key, mid_pid);
	readEntryNonLeaf(prev_record, prev_key, prev_pid);
	
	if(key >= prev_key && key <= mid_key){
		//Newly inserted record is the middle one
		midKey = key;
		//Move the right half of current node into the sibling node
		memmove(next_p,p+(NON_LEAF_ENTRY_SIZE*mid_record),(no_of_records-mid_record)* NON_LEAF_ENTRY_SIZE);
	} else if(key < mid_key) {
		//Move from key + 1 entries into sibling node and insert new key in current node.
		memmove(next_p,p+(NON_LEAF_ENTRY_SIZE*(mid_record+1)),(no_of_records-(mid_record+1))* NON_LEAF_ENTRY_SIZE);
		insert(key, pid);
		midKey = mid_key;
	} else if(key>=mid_key){
		//Move from key + 1 entries into sibling node and insert new key in sibling node.
		memmove(next_p,p+(NON_LEAF_ENTRY_SIZE*(mid_record+1)),(no_of_records-(mid_record+1))* NON_LEAF_ENTRY_SIZE);
		sibling.insert(key, pid);
		midKey = mid_key;
	}
	
	//memcpy(&midKey,next_p+PAGE_ID_SIZE,KEY_SIZE);
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
	int key_size = sizeof(int);
  	int page_id_size = sizeof(PageId);
  	int entry_size = key_size+page_id_size;
	for(curr_eid=1;curr_eid<=no_of_records;curr_eid+=1) {
		if(rc=readEntryNonLeaf(curr_eid,curr_key,curr_pid)<0) {
			return rc;
		}
		if(searchKey >= curr_key) {
			memcpy(&pid,p+(curr_eid-1)*entry_size,sizeof(PageId));
			return 0;
		}
		
	}
	return RC_NO_SUCH_RECORD;
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
	if(rc=insert(key,pid1) < 0){
		return rc;
	}
	memcpy(p+NON_LEAF_ENTRY_SIZE,&pid2,PAGE_ID_SIZE);
	return 0; 
}