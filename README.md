# hash-tailer

A little project investigating possible improvements to some hash functions for short strings

### Goals
When looking at SpookyHash and t1ha hashes, I noticed this code pattern,
in various forms.

~~~~
      switch (tail & 7) {
        /* For most CPUs this code is better when not needed
       * copying for alignment or byte reordering. */
      case 0:
        return fetch64_le(p);
      case 7:
        r = (uint64_t)p[6] << 8;
      case 6:
        r += p[5];
        r <<= 8;
      case 5:
        r += p[4];
        r <<= 32;
      case 4:
        return r + fetch32_le(p);
      case 3:
        r = (uint64_t)p[2] << 16;
      case 2:
        return r + fetch16_le(p);
      case 1:
        return p[0];
      }
~~~~

It's used to process very short pieces of data, either when the
input is short, or simply at the end of a longer stream.

It felt like something could be optimized here.

So I had this idea of trying to use full-integer memory reads.
Of course, one needs to avoid reading uninitialized memory - but if
we detect we're close to the page boundary, we can do it.

So I implemented a 16- and 8- byte version of this code, just for fun.

Here's an example for the 8-byte version:

~~~~
/** Read up to 8 bytes, save the content in resA */
static inline void tailer_mz8(ub1* message, size_t len)
{
  if (len == 0) {
    resA = 0;
    return;
  }

  if ( ((ub8)(message)) & (PAGESIZE - 8)) {
    // Common case, not at the beginning of the page.
    // Reading "junk" before the pointer is OK.
    //  A bit simpler code

    ub1 *ptr = message - (8 - len);
    resA = (*((ub8*)(ptr))) >> (8 * (8 - len));
  } else {
    // Rare case
    // Beginning of the page, need to read after the pointer and mask
    ub1 *ptr = message;
    resA = (*((ub8 *) (ptr))) & ((~0ULL) >> (8 * (8 - len)));
  }
}
~~~~

### Installation

Just run

~~~~
cmake . && make && ./hash-tailer
~~~~

### Results:

~~~~
Testing 16-byte correctness...OK

0..15 bytes, best of 10 reps: SPOOKY: 111368 us, 11.137 ns avg   VS    MZ16:  51135 us,  5.114 ns avg,  2.178x faster
0.. 7 bytes, best of 10 reps: SPOOKY: 103541 us, 10.354 ns avg   VS    MZ16:  28566 us,  2.857 ns avg,  3.625x faster
0.. 3 bytes, best of 10 reps: SPOOKY:  90679 us,  9.068 ns avg   VS    MZ16:  34870 us,  3.487 ns avg,  2.600x faster
0.. 1 bytes, best of 10 reps: SPOOKY:  67563 us,  6.756 ns avg   VS    MZ16:  48039 us,  4.804 ns avg,  1.406x faster

Testing 8-byte correctness...OK

0.. 7 bytes, best of 10 reps:   T1HA:  99436 us,  9.944 ns avg   VS     MZ8:  31074 us,  3.107 ns avg,  3.200x faster
0.. 3 bytes, best of 10 reps:   T1HA:  81821 us,  8.182 ns avg   VS     MZ8:  41898 us,  4.190 ns avg,  1.953x faster
0.. 1 bytes, best of 10 reps:   T1HA:  51191 us,  5.119 ns avg   VS     MZ8:  56325 us,  5.633 ns avg,  0.909x faster
~~~~

Looks promising.

MZ versions are pretty good, except for MZ8 on 0..1 byte inputs - hello, branch predictors?
