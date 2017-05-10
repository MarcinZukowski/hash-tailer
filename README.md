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


### Installation

Just run

~~~~
cmake . && make && ./hash-tailer
~~~~

### Results:

~~~~
Testing 16-byte correctness...OK

PERF(up to 15 bytes, best of 10 reps): SPOOKY:  102642 us, 10.264 ns avg   VS    MZ16:   48455 us,  4.845 ns avg ,  2.118x faster
PERF(up to  7 bytes, best of 10 reps): SPOOKY:   97287 us,  9.729 ns avg   VS    MZ16:   27319 us,  2.732 ns avg ,  3.561x faster
PERF(up to  3 bytes, best of 10 reps): SPOOKY:   85731 us,  8.573 ns avg   VS    MZ16:   33501 us,  3.350 ns avg ,  2.559x faster
PERF(up to  1 bytes, best of 10 reps): SPOOKY:   64003 us,  6.400 ns avg   VS    MZ16:   45730 us,  4.573 ns avg ,  1.400x faster

Testing 8-byte correctness...OK

PERF(up to  7 bytes, best of 10 reps):   T1HA:   92534 us,  9.253 ns avg   VS     MZ8:   32094 us,  3.209 ns avg ,  2.883x faster
PERF(up to  3 bytes, best of 10 reps):   T1HA:   76086 us,  7.609 ns avg   VS     MZ8:   42284 us,  4.228 ns avg ,  1.799x faster
PERF(up to  1 bytes, best of 10 reps):   T1HA:   47107 us,  4.711 ns avg   VS     MZ8:   55028 us,  5.503 ns avg ,  0.856x faster
~~~~

Looks promising.

MZ versions are pretty good, except for MZ8 on 0..1 byte inputs - hello, branch predictors?