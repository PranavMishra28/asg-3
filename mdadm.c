#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

uint32_t getOP(int command, int diskID, int blockID)
{
  uint32_t op = 0;
  op = command << 26;
  op |= diskID << 22;
  op |= blockID;

  return op;
}

int unmount = 1; // Created a global variable to check if the disk has been
                 // unmounted or not. At this moment, by defining it to be 1,
                 // we say that the disk is unmounted.

int mdadm_mount(void)
{
  if (unmount == 1)
  {
    uint32_t op = getOP(JBOD_MOUNT, 0, 0);
    int isNum = jbod_operation(op, NULL);
    if (isNum == 0)
    {
      unmount = 0;
      return 1;
    }
  }
  else
  {
    return -1;
  }
  return -1;
}

int mdadm_unmount(void)
{
  if (unmount == 1)
  { // Checks if the disk is mounted already
    return -1;
  }
  else
  {
    uint32_t op = getOP(JBOD_UNMOUNT, 0, 0);
    int isNum = jbod_operation(op, NULL);
    if (isNum == 0)
    {
      unmount = 1;
      return 1;
    }
  }
  return -1;
}

int min(int a, int b){
  if(a < b){
    return a;
  }
  else{
    return b;
  }
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf){

  // Checking for all the edge cases. If any of the cases fail we would return
  // -1 and exist out of the function.+
  // 1) If the disk is unmounted.
  // 2) If read goes beyond the length of  the disk.
  // 3) If addr is greater than 1024.
  // 4) If a NULL pointer is given as the place to read and the addr is also 0 at the same time.

  if(buf == NULL && len == 0){
    return 0;
  }

  if (len > 1024 || unmount == 1 || (buf == NULL && len < 0) || (addr + len > JBOD_DISK_SIZE * JBOD_NUM_DISKS) || (buf == NULL && len > 0) || len < 0){
    return -1;
  }

  // After reading the command fields, following this order of methods to run out operations: 
  // 1) SEEK TO DISK 2) SEEK TO BLOCK 3) READ TO THE BLOCK

  uint8_t temp_buf[256]; // We create a temporary buffer function to read in data

  int bytesRead = 0;

  while(len > 0){    
    int offset = addr % 256;               // Calculating the offset
    uint32_t currentDisk = addr/65536;     // Calculates the current disk at which the operation is at
    uint32_t blockID = (addr / 256) % 256; // Gives the blockID belonging to the block of the specific disk
    uint32_t op;                           // Helps utilize the helper function

    op = getOP(JBOD_SEEK_TO_DISK, currentDisk, 0);
    if (jbod_operation(op, NULL) == -1){
      return -1;
    }

    op = getOP(JBOD_SEEK_TO_BLOCK, 0, blockID);
    if (jbod_operation(op, NULL) == -1){
      return -1;
    }       

    jbod_operation(getOP(JBOD_READ_BLOCK, 0, 0), temp_buf);

    int bytesWrite = min(len, min(256, 256-offset));

    memcpy(buf + bytesRead, temp_buf + offset, bytesWrite);

    bytesRead += bytesWrite;
    len -= bytesWrite;
    addr += bytesWrite;
  }
  return bytesRead;
}


int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
  
  int toWrite = 0;

  // Check for invalid input values
  if(buf == NULL && len == 0){
    return 0;
  }

  if(len > 1024 || unmount == 1 || (buf == NULL && len < 0) || (addr + len > JBOD_DISK_SIZE * JBOD_NUM_DISKS) || (buf == NULL && len > 0) || len < 0){
    return -1;
  }

  uint8_t temp_buf1[256]; // We create a temporary buffer function to read in data

  while(toWrite < len){

    int offset = addr % 256;                 // Calculate the offset
    uint32_t currentDisk = addr/65536;       // Calculate the current disk
    uint32_t blockID = (addr / 256) % 256;   // Calculate the block ID
    uint32_t op;                             // Helper function for the helper function

    uint8_t temp_buf2[256]; // Write required data from buf and temp_buf1 into temp_buf2

    // Seek to the current disk
    op = getOP(JBOD_SEEK_TO_DISK, currentDisk, 0);
    if (jbod_operation(op, NULL) == -1){
      return -1;
    }

    // Seek to the current block
    op = getOP(JBOD_SEEK_TO_BLOCK, currentDisk, blockID);
    if (jbod_operation(op, NULL) == -1){
      return -1;
    }

    // Reading the block
    op = getOP(JBOD_READ_BLOCK, currentDisk, blockID);
    if (jbod_operation(op, temp_buf1) == -1){
      return -1;
    }

    // Seek to the current block
    op = getOP(JBOD_SEEK_TO_BLOCK, currentDisk, blockID); // The I/O operation increments the block count, in order to start fresh from the first block to have data written into we seek into the first block again.
    if (jbod_operation(op, NULL) == -1){
      return -1;
    }

    int bytesWrite = min(len, min(256,256-offset)); // Number of bytes to be written in the block.

    memcpy(temp_buf2, temp_buf1, offset); // Copying the number of bytes from temp_buf1 to temp_buf2
    memcpy(temp_buf2 + offset, buf + toWrite, bytesWrite); // Copying buf to temp_buf1
    memcpy(temp_buf2 + offset + bytesWrite, temp_buf1 + offset + bytesWrite, 256 - (offset + bytesWrite)); // We dont leave the rest of the temp_buf2 empty

    op = getOP(JBOD_WRITE_BLOCK, currentDisk, blockID);
    if(jbod_operation(op, temp_buf2) == -1){
      return -1;
    }

    toWrite += bytesWrite; // Total bytes written
    addr += bytesWrite; // The next address which has to undergo all these operations
    offset = 0; // Refreshes the offset
    // printf("len: %u toWrite: %u", len, toWrite);
  }
  return len;
}