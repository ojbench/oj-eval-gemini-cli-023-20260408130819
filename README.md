# Deque Implementation

## Split and Merge Strategy

This deque is implemented using an Unrolled Linked List (a doubly linked list of circular arrays). Each block (node) in the linked list contains a circular array of maximum capacity `MAX_CAP = 512`.

### Split Strategy
When a block becomes full (`sz == MAX_CAP`), it is split into two blocks. The first block retains the first half of the elements (`MAX_CAP / 2`), and the second block receives the second half. This ensures that after a split, both blocks have enough free space for future insertions, preventing cascading splits.

### Merge Strategy
When an element is erased from a block, its size decreases. If the size drops below `MAX_CAP / 2`, we attempt to maintain the block's occupancy to prevent having too many small blocks (which would degrade random access performance):
1. **Merge with Next**: If the next block exists and their combined size is strictly less than `MAX_CAP`, we merge the current block with the next block.
2. **Merge with Prev**: If the previous block exists and their combined size is strictly less than `MAX_CAP`, we merge the previous block with the current block.
3. **Borrow from Next**: If merging is not possible but the next block exists, it means the next block has more than `MAX_CAP / 2` elements. We borrow one element from the front of the next block and add it to the back of the current block.
4. **Borrow from Prev**: If the previous block exists, we borrow one element from the back of the previous block and add it to the front of the current block.

## Time Complexity Analysis

- **Amortized $O(1)$ Push/Pop at Ends**: 
  Because each block uses a circular array, `push_front`, `push_back`, `pop_front`, and `pop_back` on a block take $O(1)$ time. When a block is full, splitting takes $O(B)$ time (where $B = 	ext{MAX_CAP}$). However, since a split leaves both blocks half-full, it takes at least $B/2$ operations before another split or merge occurs on these blocks. Thus, the $O(B)$ cost is amortized over $O(B)$ operations, resulting in an amortized $O(1)$ time complexity for insertions and deletions at the ends.

- **Worst-case $O(\sqrt{n})$ Random Access**:
  To access the $i$-th element, we traverse the linked list of blocks. Since each block (except possibly the only block) is guaranteed to have at least $B/2$ elements, the number of blocks is at most $2n/B + 1$. Traversing the blocks takes $O(n/B)$ time. By choosing $B \approx \sqrt{n}$ (or a constant like 512 which balances cache efficiency and traversal time), the traversal takes $O(\sqrt{n})$ time. Once the correct block is found, accessing the element in the circular array takes $O(1)$ time.

- **Worst-case $O(\sqrt{n})$ Random Insertion/Deletion**:
  Finding the correct block takes $O(n/B)$ time. Inserting or erasing an element within the circular array requires shifting at most $B/2$ elements, which takes $O(B)$ time. Splitting, merging, or borrowing takes $O(B)$ time. The total time is $O(n/B + B)$. With $B$ chosen appropriately, this is bounded by $O(\sqrt{n})$.