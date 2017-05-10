#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#include <string>
#include <cstring>

typedef  uint64_t  uint64;
typedef  uint32_t  uint32;
typedef  uint16_t  uint16;
typedef  uint8_t   uint8;

typedef  uint64_t  ub8;
typedef  uint32_t  ub4;
typedef  uint16_t  ub2;
typedef  uint8_t   ub1;

// For debugging when lazy
//#define pr printf
#define pr

// Used to return results
uint64 resA;
uint64 resB;

/** Return current time in usec */
ub8 usec()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return ((ub8)tv.tv_sec) * 1000000 + tv.tv_usec;
}

/** ************************************** SPOOKY ******************************* */
namespace spooky
{
/**
 * This code is based on SpookyHash::Short, extracted only the part processing the tail 16 bytes.
 */

static inline void tailer_spooky(const ub1 *message, size_t length)
{
//  static constexpr uint64 sc_const = 0xdeadbeefdeadbeefLL;
  static constexpr uint64 sc_const = 0LL;  // MZ: modified to be compatible with others

  union
  {
    const uint8 *p8;
    uint32 *p32;
    uint64 *p64;
    size_t i;
  } u;

  u.p8 = (const uint8 *) message;

  uint64 c = sc_const;
  uint64 d = sc_const;

  // Handle the last 0..15 bytes, and its length
  switch (length)
  {
    case 15:
      d += ((uint64) u.p8[14]) << 48;
    case 14:
      d += ((uint64) u.p8[13]) << 40;
    case 13:
      d += ((uint64) u.p8[12]) << 32;
    case 12:
      d += u.p32[2];
      c += u.p64[0];
      break;
    case 11:
      d += ((uint64) u.p8[10]) << 16;
    case 10:
      d += ((uint64) u.p8[9]) << 8;
    case 9:
      d += (uint64) u.p8[8];
    case 8:
      c += u.p64[0];
      break;
    case 7:
      c += ((uint64) u.p8[6]) << 48;
    case 6:
      c += ((uint64) u.p8[5]) << 40;
    case 5:
      c += ((uint64) u.p8[4]) << 32;
    case 4:
      c += u.p32[0];
      break;
    case 3:
      c += ((uint64) u.p8[2]) << 16;
    case 2:
      c += ((uint64) u.p8[1]) << 8;
    case 1:
      c += (uint64) u.p8[0];
      break;
    case 0:
      c += sc_const;
      d += sc_const;
  }

  resA = c;
  resB = d;
}

} // namespace spooky


/** ************************************** T1HA ******************************* */
namespace t1ha
{
/**
 * Again, based on t1ha code, focused on tail64_le
 */
static __inline uint64_t fetch64_le(const void *v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return *(const uint64_t *)v;
#else
  return bswap64(*(const uint64_t *)v);
#endif
}

static __inline uint32_t fetch32_le(const void *v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return *(const uint32_t *)v;
#else
  return bswap32(*(const uint32_t *)v);
#endif
}

static __inline uint16_t fetch16_le(const void *v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return *(const uint16_t *)v;
#else
  return bswap16(*(const uint16_t *)v);
#endif
}

static __inline uint64_t tail64_le(const void *v, size_t tail) {
  const uint8_t *p = (const uint8_t *)v;
  uint64_t r = 0;
  switch (tail & 7) {
#if UNALIGNED_OK && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
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
#else
    /* For most CPUs this code is better than a
     * copying for alignment and/or byte reordering. */
    case 0:
      r = p[7] << 8;
    case 7:
      r += p[6];
      r <<= 8;
    case 6:
      r += p[5];
      r <<= 8;
    case 5:
      r += p[4];
      r <<= 8;
    case 4:
      r += p[3];
      r <<= 8;
    case 3:
      r += p[2];
      r <<= 8;
    case 2:
      r += p[1];
      r <<= 8;
    case 1:
      return r + p[0];
#endif
  }
  assert(0);
}

/**
 * Little wrapper on tail64_le that handles 0-length data
 */
static inline void tailer_t1ha(ub1 *message, size_t len)
{
  if (!len)
  {
    resA = 0;
    return;
  }
  else
  {
    resA = tail64_le(message, len);
  }
}

}  // namespace t1ha

/** ************************************** MZ ******************************* */
namespace mz
{
/**
 * My version for 16-byte tail
 */
static inline void tailer_mz16(ub1* message, size_t len)
{
  pr("%p %2d ",  message, len);

  if (len == 0)
  {
    pr("0\n");
    resA = 0;
    resB = 0;
    return;
  }

  if ( ((ub8)(message + 15))& (4096 - 15))
  {
    // Common case
    // Reading after the address is fine
    ub1 *ptr = message;
    if (len <= 8)
    {
      pr("c\n");
      resA = (*((ub8*)(ptr))) & ((~0ULL) >> (8 * (8 - len)));
      resB = 0;
    }
    else
    {
      pr("d\n");
      resA = *((ub8*)(ptr));
      resB = (*((ub8*)(ptr + 8))) & ((~0ULL) >> (8 * (16 - len)));
    }
  }
  else
  {
    // Rare case
    // Can cross the page boundary, need to read from before it
    // Reading before the address is fine
    ub1 *ptr = message - (16 - len);
    if (len <= 8)
    {
      pr("a\n");
      resA = (*((ub8*)(ptr + 8))) >> (8 * (8 - len));
      resB = 0;
    }
    else
    {
      pr("b\n");
      ub8 tmp = *((ub8*)(ptr + 8));
      resA = *((ub8*)(ptr)) >> (8*(16-len)) | (tmp << (8*(len-8)));
      resB = tmp >> (8*(8-len));
    }
  }
}

/** And for 8-byte tail */
static inline void tailer_mz8(ub1* message, size_t len)
{
  pr("%p %2d ",  message, len);

  if (len == 0)
  {
    pr("0\n");
    resA = 0;
    return;
  }

  if ( ((ub8)(message + 7))& (4096 - 7))
  {
    // Common case
    // Reading "junk" after the data is fine
    pr("c\n");
    ub1 *ptr = message;
    resA = (*((ub8 *) (ptr))) & ((~0ULL) >> (8 * (8 - len)));
  }
  else
  {
    // Rare case
    // Can cross the page boundary, need to read from before the pointer
    // (that is guaranteed to be fine ;))
    ub1 *ptr = message - (8 - len);
    pr("a\n");
    resA = (*((ub8*)(ptr))) >> (8 * (8 - len));
  }
}

}  // namespace mz



constexpr int PAGESIZE = 4096;
/** Our data buffer */
ub1 buf[PAGESIZE + 16];

/* Number of performance runs */
constexpr ub8 PRUNS = 10 * 1000 * 1000;

/* Number of reps to find the best possible time */
constexpr int REPS = 5;

// We store inputs to the functions in a table, so the compiler can't be too smart, otherwise
// it can e.g. derive that some branches are always true when e.g. len is MOD 2
constexpr ub8 PRUNS_BUFSIZE = 10000;
ub1 lens[PRUNS_BUFSIZE];
ub2 offs[PRUNS_BUFSIZE];

/** Run a single function REPS * PRUNS times */
template <typename Func>
void test_performance_single(Func func, ub8 *out_time, ub8 *out_sum)
{
  ub8 sum = 0;
  ub8 best_time = 0;

  for (int r = 0; r < REPS; r++)
  {
    ub8 start = usec();
    for (ub8 i = 0; i < PRUNS; i++) {
      func(buf + offs[i % PRUNS_BUFSIZE], lens[i % PRUNS_BUFSIZE]);
      sum += resA + resB;
    }
    ub8 t = usec() - start;

    if (r == 0 || t < best_time)
      best_time = t;
  }

  *out_time = best_time;
  *out_sum = sum;
}

template <typename Func1, typename Func2>
void test_performance(Func1 func1, const char *name1, Func2 func2, const char *name2, int bytes)
{
  ub8 time1, sum1;
  ub8 time2, sum2;
  ub8 time3, sum3;
  ub8 time4, sum4;

  // Prepare the buffers with our function call parameters
  srand(0);
  for (int i = 0; i < PRUNS_BUFSIZE; i++) {
    lens[i] = (ub1)(rand() % bytes);
    offs[i] = (ub2)(rand() % PAGESIZE);
  }

  // Run functions twice, avoiding warm-up issues
  test_performance_single(func1, &time1, &sum1);
  test_performance_single(func2, &time2, &sum2);
  test_performance_single(func1, &time3, &sum3);
  test_performance_single(func2, &time4, &sum4);

  time1 = std::min(time1, time3);
  time2 = std::min(time2, time4);

  if (sum1 != sum2) {
    printf("ERROR: %s VS %s,   %llu <> %llu\n", name1, name2, sum1, sum2);
    exit(1);
  }

  printf("PERF(up to %2d bytes, best of %d reps): %6s: %7lld us, %6.3f ns avg   VS  %6s: %7lld us, %6.3f ns avg , %6.3fx faster\n",
         bytes - 1, 2 * REPS,
         name1, time1, 1000.0f * time1 / PRUNS,
         name2, time2, 1000.0f * time2 / PRUNS,
         1.0f * time1 / time2);
};

using spooky::tailer_spooky;
using t1ha::tailer_t1ha;
using mz::tailer_mz8;
using mz::tailer_mz16;

template <typename Func1, typename Func2>
void test_correctness(Func1 func1, const char *name1, Func2 func2, const char *name2, int bytes)
{
  // Test correctness
  printf("\nTesting %d-byte correctness...", bytes);

  for (int i = 0; i < bytes * PAGESIZE; i++)
  {
    int len = i / PAGESIZE;
    int off = i % PAGESIZE;

    func1(buf + off, len);

    uint64 a1 = resA;
    uint64 b1 = resB;

    func2((ub1 *) (buf + off), len);

    uint64 a2 = resA;
    uint64 b2 = resB;

    if (a1 != a2|| b1 != b2)
    {
      printf("ERROR on\n  Data: ");
      for (int i = 0; i < len; i++)
      {
        printf("%02x ", (ub1) buf[off + i]);
      }
      printf("\n  ERROR: off=%d len=%d %s: %016llx %016llx   <>   %s: %016llx %016llx\n",
             off, len,
             name1, a1, b1,
             name2, a2, b2);
      exit(-1);
    }
  }
  printf("OK\n\n");
};



void test16()
{
  test_correctness(tailer_spooky, "SPOOKY", tailer_mz16, "MZ16", 16);

  test_performance(tailer_spooky, "SPOOKY", tailer_mz16, "MZ16", 16);
  test_performance(tailer_spooky, "SPOOKY", tailer_mz16, "MZ16", 8);
  test_performance(tailer_spooky, "SPOOKY", tailer_mz16, "MZ16", 4);
  test_performance(tailer_spooky, "SPOOKY", tailer_mz16, "MZ16", 2);
}

void test8()
{
  test_correctness(tailer_t1ha, "T1HA", tailer_mz8, "MZ8", 8);

  test_performance(tailer_t1ha, "T1HA", tailer_mz8, "MZ8", 8);
  test_performance(tailer_t1ha, "T1HA", tailer_mz8, "MZ8", 4);
  test_performance(tailer_t1ha, "T1HA", tailer_mz8, "MZ8", 2);
}

int main()
{

  // Fill with random data
  srand(1);
  for (int i = 0; i < PAGESIZE; i++)
  {
    buf[i] = (ub1) rand();
  }

  test16();
  test8();

  return 0;
}


