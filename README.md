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

0..15 bytes, best of 10 reps: SPOOKY: 112018 us, 11.202 ns avg   VS    MZ16:  49574 us,  4.957 ns avg,  2.260x faster
0.. 7 bytes, best of 10 reps: SPOOKY: 103978 us, 10.398 ns avg   VS    MZ16:  28925 us,  2.892 ns avg,  3.595x faster
0.. 3 bytes, best of 10 reps: SPOOKY:  92090 us,  9.209 ns avg   VS    MZ16:  36545 us,  3.655 ns avg,  2.520x faster
0.. 1 bytes, best of 10 reps: SPOOKY:  69080 us,  6.908 ns avg   VS    MZ16:  49085 us,  4.909 ns avg,  1.407x faster

Testing 8-byte correctness...OK

0.. 7 bytes, best of 10 reps:   T1HA:  97800 us,  9.780 ns avg   VS     MZ8:  34489 us,  3.449 ns avg,  2.836x faster
0.. 3 bytes, best of 10 reps:   T1HA:  81647 us,  8.165 ns avg   VS     MZ8:  46616 us,  4.662 ns avg,  1.751x faster
0.. 1 bytes, best of 10 reps:   T1HA:  48696 us,  4.870 ns avg   VS     MZ8:  57028 us,  5.703 ns avg,  0.854x faster
~~~~

Looks promising.

MZ versions are pretty good, except for MZ8 on 0..1 byte inputs - hello, branch predictors?