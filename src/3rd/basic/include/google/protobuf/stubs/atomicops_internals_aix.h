
#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_AIX_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_AIX_H_

namespace google {
namespace protobuf {
namespace internal {

inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  Atomic32 result;

  asm volatile (
      "1:     lwarx %[res], %[zero], %[obj]   \n\t"  // load and reserve
      "       cmpw %[cmp], %[res]             \n\t"  // compare values
      "       bne- 2f                         \n\t"
      "       stwcx. %[val], %[zero], %[obj]  \n\t"  // store new value
      "       bne- 1b                         \n\t"
      "2:                                     \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [cmp]  "b"   (old_value),
                [val]  "b"   (new_value),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                         Atomic32 new_value) {
  Atomic32 result;

  asm volatile (
      "1:     lwarx %[res], %[zero], %[obj]       \n\t"
      "       stwcx. %[val], %[zero], %[obj]      \n\t"
      "       bne- 1b                             \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [val]  "b"   (new_value),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
                                          Atomic32 increment) {
  Atomic32 result;

  asm volatile (
      "1:     lwarx %[res], %[zero], %[obj]   \n\t"  // load and reserve
      "       add %[res], %[val], %[res]      \n\t"  // add the operand
      "       stwcx. %[res], %[zero], %[obj]  \n\t"  // store old value
                                                     // if still reserved
      "       bne- 1b                         \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [val]  "b"   (increment),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline void MemoryBarrier(void) {
  asm volatile (
      "       lwsync                          \n\t"
      "       isync                           \n\t"
              :
              :
              : "memory");
}

inline Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                        Atomic32 increment) {
  Atomic32 result;

  asm volatile (
      "       lwsync                          \n\t"

      "1:     lwarx %[res], %[zero], %[obj]   \n\t"  // load and reserve
      "       add %[res], %[val], %[res]      \n\t"  // add the operand
      "       stwcx. %[res], %[zero], %[obj]  \n\t"  // store old value
                                                     // if still reserved
      "       bne- 1b                         \n\t"
      "       isync                           \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [val]  "b"   (increment),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  Atomic32 result;

  asm volatile (
      "1:     lwarx %[res], %[zero], %[obj]   \n\t"  // load and reserve
      "       cmpw %[cmp], %[res]             \n\t"  // compare values
      "       bne- 2f                         \n\t"
      "       stwcx. %[val], %[zero], %[obj]  \n\t"  // store new value
      "       bne- 1b                         \n\t"

      "       isync                           \n\t"
      "2:                                     \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [cmp]  "b"   (old_value),
                [val]  "b"   (new_value),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  Atomic32 result;

  asm volatile (
      "       lwsync                          \n\t"

      "1:     lwarx %[res], %[zero], %[obj]   \n\t"  // load and reserve
      "       cmpw %[cmp], %[res]             \n\t"  // compare values
      "       bne- 2f                         \n\t"
      "       stwcx. %[val], %[zero], %[obj]  \n\t"  // store new value
      "       bne- 1b                         \n\t"

      "2:                                     \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [cmp]  "b"   (old_value),
                [val]  "b"   (new_value),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
}

inline void Acquire_Store(volatile Atomic32* ptr, Atomic32 value) {
  asm volatile (
      "       stw %[val], %[obj]      \n\t"
      "       isync                   \n\t"
              : [obj] "=m" (*ptr)
              : [val]  "b"  (value));
}

inline void Release_Store(volatile Atomic32* ptr, Atomic32 value) {
  asm volatile (
      "       lwsync                  \n\t"
      "       stw %[val], %[obj]      \n\t"
              : [obj] "=m" (*ptr)
              : [val]  "b"  (value));
}

inline Atomic32 NoBarrier_Load(volatile const Atomic32* ptr) {
  return *ptr;
}

inline Atomic32 Acquire_Load(volatile const Atomic32* ptr) {
  Atomic32 result;

  asm volatile (
      "1:     lwz %[res], %[obj]              \n\t"
      "       cmpw %[res], %[res]             \n\t" // create data
                                                    // dependency for
                                                    // load/load ordering
      "       bne- 1b                         \n\t" // never taken

      "       isync                           \n\t"
              : [res]  "=b" (result)
              : [obj]  "m"  (*ptr),
                [zero] "i"  (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic32 Release_Load(volatile const Atomic32* ptr) {
  Atomic32 result;

  asm volatile (
      "       lwsync                          \n\t"

      "1:     lwz %[res], %[obj]              \n\t"
      "       cmpw %[res], %[res]             \n\t" // create data
                                                    // dependency for
                                                    // load/load ordering
      "       bne- 1b                         \n\t" // never taken
              : [res]  "=b" (result)
              : [obj]  "m"  (*ptr),
                [zero] "i"  (0)
              : "cr0", "ctr");

  return result;
}

#ifdef GOOGLE_PROTOBUF_ARCH_64_BIT
inline Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
                                         Atomic64 old_value,
                                         Atomic64 new_value) {
  Atomic64 result;

  asm volatile (
      "1:     ldarx %[res], %[zero], %[obj]   \n\t"  // load and reserve
      "       cmpd %[cmp], %[res]             \n\t"  // compare values
      "       bne- 2f                         \n\t"

      "       stdcx. %[val], %[zero], %[obj]  \n\t"  // store the new value
      "       bne- 1b                         \n\t"
      "2:                                     \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [cmp]  "b"   (old_value),
                [val]  "b"   (new_value),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr,
                                         Atomic64 new_value) {
  Atomic64 result;

  asm volatile (
      "1:     ldarx %[res], %[zero], %[obj]       \n\t"
      "       stdcx. %[val], %[zero], %[obj]      \n\t"
      "       bne- 1b                             \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [val]  "b"   (new_value),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr,
                                          Atomic64 increment) {
  Atomic64 result;

  asm volatile (
      "1:     ldarx %[res], %[zero], %[obj]   \n\t" // load and reserve
      "       add %[res], %[res], %[val]      \n\t" // add the operand
      "       stdcx. %[res], %[zero], %[obj]  \n\t" // store old value if
                                                    // still reserved

      "       bne- 1b                         \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [val]  "b"   (increment),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr,
                                        Atomic64 increment) {

  Atomic64 result;

  asm volatile (
      "       lwsync                          \n\t"

      "1:     ldarx %[res], %[zero], %[obj]   \n\t" // load and reserve
      "       add %[res], %[res], %[val]      \n\t" // add the operand
      "       stdcx. %[res], %[zero], %[obj]  \n\t" // store old value if
                                                    // still reserved

      "       bne- 1b                         \n\t"

      "       isync                           \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [val]  "b"   (increment),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  Atomic64 result;

  asm volatile (
      "1:     ldarx %[res], %[zero], %[obj]   \n\t"  // load and reserve
      "       cmpd %[cmp], %[res]             \n\t"  // compare values
      "       bne- 2f                         \n\t"

      "       stdcx. %[val], %[zero], %[obj]  \n\t"  // store the new value
      "       bne- 1b                         \n\t"
      "       isync                           \n\t"
      "2:                                     \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [cmp]  "b"   (old_value),
                [val]  "b"   (new_value),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic64 Release_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  Atomic64 result;

  asm volatile (
      "       lwsync                          \n\t"

      "1:     ldarx %[res], %[zero], %[obj]   \n\t"  // load and reserve
      "       cmpd %[cmp], %[res]             \n\t"  // compare values
      "       bne- 2f                         \n\t"

      "       stdcx. %[val], %[zero], %[obj]  \n\t"  // store the new value
      "       bne- 1b                         \n\t"
      "2:                                     \n\t"
              : [res]  "=&b" (result)
              : [obj]  "b"   (ptr),
                [cmp]  "b"   (old_value),
                [val]  "b"   (new_value),
                [zero] "i"   (0)
              : "cr0", "ctr");

  return result;
}

inline void NoBarrier_Store(volatile Atomic64* ptr, Atomic64 value) {
  *ptr = value;
}

inline void Acquire_Store(volatile Atomic64* ptr, Atomic64 value) {
  asm volatile (
      "       std %[val], %[obj]          \n\t"
      "       isync                       \n\t"
              : [obj] "=m" (*ptr)
              : [val] "b"  (value));
}

inline void Release_Store(volatile Atomic64* ptr, Atomic64 value) {
  asm volatile (
      "       lwsync                      \n\t"
      "       std %[val], %[obj]          \n\t"
              : [obj] "=m" (*ptr)
              : [val] "b"  (value));
}

inline Atomic64 NoBarrier_Load(volatile const Atomic64* ptr) {
  return *ptr;
}

inline Atomic64 Acquire_Load(volatile const Atomic64* ptr) {
  Atomic64 result;

  asm volatile (
      "1:     ld %[res], %[obj]                   \n\t"
      "       cmpd %[res], %[res]                 \n\t" // create data
                                                        // dependency for
                                                        // load/load ordering
      "       bne- 1b                             \n\t" // never taken

      "       isync                               \n\t"
              : [res]  "=b" (result)
              : [obj]  "m"  (*ptr),
                [zero] "i"  (0)
              : "cr0", "ctr");

  return result;
}

inline Atomic64 Release_Load(volatile const Atomic64* ptr) {
  Atomic64 result;

  asm volatile (
      "       lwsync                              \n\t"

      "1:     ld %[res], %[obj]                   \n\t"
      "       cmpd %[res], %[res]                 \n\t" // create data
                                                        // dependency for
                                                        // load/load ordering
      "       bne- 1b                             \n\t" // never taken
              : [res]  "=b" (result)
              : [obj]  "m"  (*ptr),
                [zero] "i"  (0)
              : "cr0", "ctr");

  return result;
}
#endif

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_SPARC_GCC_H_
