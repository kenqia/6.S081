// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define TABLESIZE 17 

struct {
  struct spinlock lock[TABLESIZE];
  struct buf bucket[TABLESIZE];
  
  struct buf buf[NBUF];
  struct spinlock eviction_lock;
} bcache;

int hash(int key){
    return (key % TABLESIZE);
}


void
binit(void)
{
  struct buf *b;

  initlock(&bcache.eviction_lock, "bcache_eviction");
  for(int i = 0 ; i < TABLESIZE ; i++){
    initlock(&bcache.lock[i], "bcache");
    bcache.bucket[i].next = 0;
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->valid = 0;      
    b->dev = 0;        
    b->blockno = 0;    
    b->tick = 0;
    b->refcnt = 0;
    b->next = bcache.bucket[0].next;
    bcache.bucket[0].next = b;
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int pos = hash(blockno);
  acquire(&bcache.lock[pos]);
  
  // Is the block already cached?
  for(b = bcache.bucket[pos].next ; b != 0 ; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->tick = ticks;
      release(&bcache.lock[pos]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  release(&bcache.lock[pos]);
  acquire(&bcache.eviction_lock);

  acquire(&bcache.lock[pos]);
  for(struct buf *t = bcache.bucket[pos].next ; t != 0 ; t = t->next){
    if(t->dev == dev && t->blockno == blockno){
      t->refcnt++;
      t->tick = ticks;
      release(&bcache.lock[pos]);
      release(&bcache.eviction_lock);
      acquiresleep(&t->lock);
      return t;
    }
  }
  release(&bcache.lock[pos]);

  struct buf *min = 0;
  int holding = -1;
  for(int i = 0 ; i < TABLESIZE ; i++){
    int found = 0;
    acquire(&bcache.lock[i]);
    b = &bcache.bucket[i];
    while(b->next != 0){
      if(b->next->refcnt == 0 && (!min || b->next->tick < min->next->tick)){
        found = 1;
        min = b;
      }
      b = b->next;
    }

    if(!found){
      release(&bcache.lock[i]);
    }else{
       if(holding != -1) release(&bcache.lock[holding]);
      holding = i;
    }
  }
  if(!min)
    panic("no free bcache");

  if(min->next->refcnt)
    panic("no free bcache");

  b = min->next;
  if(holding != pos){
    min->next = b->next;
    release(&bcache.lock[holding]);

    acquire(&bcache.lock[pos]);
    b->next = bcache.bucket[pos].next;
    bcache.bucket[pos].next = b;
  }

  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->tick = ticks;
  
  release(&bcache.lock[pos]);
  release(&bcache.eviction_lock);
  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int pos = hash(b->blockno);
  
  acquire(&bcache.lock[pos]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
  }
  release(&bcache.lock[pos]);
}

void
bpin(struct buf *b) {
  int pos = hash(b->blockno);
  acquire(&bcache.lock[pos]);
  b->refcnt++;
  release(&bcache.lock[pos]);
}

void
bunpin(struct buf *b) {
  int pos = hash(b->blockno);
  acquire(&bcache.lock[pos]);
  b->refcnt--;
  release(&bcache.lock[pos]);
}


