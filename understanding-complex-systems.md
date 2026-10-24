# Understanding Complex Systems
## From shared state to agreement

**An explanation-first course in concurrency, ownership, and distributed uncertainty**  
Expanded textbook edition · Revision 0.3 · 24 October 2026

An attempt to study topics and reconstruct what I learned into a "textbook" style writeup. 
Yes LLMs were leveraged in this endeavor: taking existing course syllabus to guide a 
focused and specific progression, and framing all learning around example programs that 
are either 'broken' or need to implement the concepts being studied. 

The organizing question is:

> What is the system entitled to believe, and what mechanism establishes that belief?

You do not need the original chats to follow the text. The discussion has
been rewritten as a continuous explanation, retaining the misconceptions,
corrections, and worked traces that made the ideas understandable. Discussion
questions are condensed from the source rather than presented as a verbatim
transcript. New controlled experiments are labeled separately.

### How to work through the book

Assume basic C functions, loops, pointers, and structures; the concurrency
concepts are developed from the beginning. Linux or WSL with a C11 compiler
is the reference execution environment. A short example can be saved using
its displayed filename and built with:

```bash
gcc -std=c11 -O2 -Wall -Wextra -pthread example.c -o example
./example
```

The assembly probe deliberately has no `main`; its chapter uses `-c` or `-S`.
The two ordinary racy counter demonstrations contain undefined behavior and
are identified as such. The teaching programs otherwise focus on the stated
protocol and usually assume successful pthread initialization and creation;
they are not complete production frameworks.

For each lab, first predict the result, then draw a permitted execution,
then run it. Keep the invariant separate from a particular winning thread or
a particular timing. Use the guided answers to check the reasoning, not only
the output. A failed speculative attempt can be how correctness is preserved.

The sixteen submitted C files are indexed in Appendix A and preserved in the
companion archive. Complete teaching versions are inline, so the Markdown
and offline HTML remain readable without opening external code files. The
source captures and exact continuation marker are retained for later additions.

## Contents

1. [Correctness under assumptions](#1-correctness-under-assumptions)
2. [Lab: the counter and the lock that was not shared](#2-lab-the-counter-and-the-lock-that-was-not-shared)
3. [Lab: compare-and-swap and the unexpected winner](#3-lab-compare-and-swap-and-the-unexpected-winner)
4. [Lab: building mutual exclusion from an atomic primitive](#4-lab-building-mutual-exclusion-from-an-atomic-primitive)
5. [Lab: correct programs can still fight over a cache line](#5-lab-correct-programs-can-still-fight-over-a-cache-line)
6. [Lab: a semaphore baton and a finish barrier](#6-lab-a-semaphore-baton-and-a-finish-barrier)
7. [Why memory and isolation have several different boundaries](#7-why-memory-and-isolation-have-several-different-boundaries)
8. [Ownership, lifetime, and the camera/MEX case study](#8-ownership-lifetime-and-the-cameramex-case-study)
9. [Lab: publishing ordinary data with acquire/release](#9-lab-publishing-ordinary-data-with-acquirerelease)
10. [Lab: atomic inventory operations, non-atomic decisions](#10-lab-atomic-inventory-operations-non-atomic-decisions)
11. [Immutability moves coordination to publication](#11-immutability-moves-coordination-to-publication)
12. [Database transactions and stale decisions](#12-database-transactions-and-stale-decisions)
13. [Lab: one seat, two buyers](#13-lab-one-seat-two-buyers)
14. [Lab: many uniquely numbered tickets](#14-lab-many-uniquely-numbered-tickets)
15. [Correct executions and state machines](#15-correct-executions-and-state-machines)
16. [Lab: consistent snapshots of multiple fields](#16-lab-consistent-snapshots-of-multiple-fields)
17. [When the validation protocol cannot be trusted](#17-when-the-validation-protocol-cannot-be-trusted)
18. [Lab: conflicting proposals and accidental double voting](#18-lab-conflicting-proposals-and-accidental-double-voting)
19. [Repairing the right critical section](#19-repairing-the-right-critical-section)
20. [Safety without progress](#20-safety-without-progress)
21. [A reliable split without undefined behavior](#21-a-reliable-split-without-undefined-behavior)
22. [Another round is not a vote on who was right](#22-another-round-is-not-a-vote-on-who-was-right)
23. [Quorum evidence and incomplete knowledge](#23-quorum-evidence-and-incomplete-knowledge)
24. [A supervisor chooses a leader; an old node does not learn it](#24-a-supervisor-chooses-a-leader-an-old-node-does-not-learn-it)
25. [Election authority and permission to commit work](#25-election-authority-and-permission-to-commit-work)
26. [From an accepted socket to a complete command](#26-from-an-accepted-socket-to-a-complete-command)
27. [Where this expanded edition stops](#27-where-this-expanded-edition-stops)

## 1. Correctness under assumptions

The course began with a precise goal: understand how systems remain correct,
safe, or recoverable when their assumptions fail. Concurrency, consensus,
security, and formal methods are different settings for that same inquiry.

An **execution** is a sequence of state transitions. A concurrent program
admits many executions because thread operations can overlap and interleave.
Correctness has to cover every execution permitted by the model, including
ones that never appeared in the few runs we observed.

An **invariant** must hold throughout the relevant execution or observation
boundary. A **postcondition** is required after an operation or computation
finishes. For a counter, `counter == 2 * ITERATIONS` after both workers join
is a postcondition; it is not something that should hold throughout their
loops. Always specify when a relationship must be observable as true.

| Term | Question to ask |
| --- | --- |
| State | What information affects the next decision? |
| Transition | What change is being attempted? |
| Invariant | What must remain true at the specified boundary? |
| Safety | What bad outcome must never be accepted? |
| Liveness | What useful outcome must eventually occur, under which assumptions? |
| Atomicity | Exactly which operation is indivisible to the relevant observers? |
| Ownership | Who has permission to access or change this state? |
| Evidence | What establishes that a decision is justified? |

“Atomic” does not mean one clock cycle, nor does it make surrounding code
atomic. Its boundary could be an increment, a conditional claim, or a larger
operation implemented by a protocol.

### The recurring lesson template

For each experiment, record:

1. **Invariant:** the property and its observation boundary.
2. **Mechanism:** the rule intended to preserve it.
3. **Assumptions:** participant behavior and environmental guarantees.
4. **Failure:** the execution that violates an assumption or property.
5. **Detection:** what evidence exposes the failure, and what remains uncertain.
6. **Containment:** what prevents one failure from spreading.
7. **Recovery:** how the system returns to useful valid operation.
8. **Adversarial angle:** whether timing or another assumption can be manipulated.
9. **Lesson:** what the next design must establish explicitly.

The initial roadmap included independent replicated processes, crash recovery,
network partitions, Raft, Byzantine protocols, model checking, and fault
injection. Those remain a roadmap, not claims of completed implementations.
This edition covers the actual C progression and its connections through the
one-slot replica simulator and the unfinished split-vote exercise.

### Why “it worked” is an incomplete observation

Suppose the program ends with the expected number. There are at least three
possible explanations: the mechanism prevented an incorrect execution; the
scheduler happened not to expose the defect; or the test did not inspect the
property that was actually violated. Those explanations demand different next
steps. More successful runs cannot, by themselves, distinguish all of them.

The first question in a useful investigation is therefore not which library
to use. It is what the program claims. “This program is thread-safe” is too
broad to test. “No ticket ID is assigned to two workers” is specific. “After
all workers finish, issued tickets plus remaining inventory equals the initial
inventory” is another claim. A mechanism might establish one without making
an arbitrary intermediate read of both counters consistent.

The word *always* needs a scope. If a transfer first subtracts from checking
and then adds to savings, memory can contain an intermediate total of 900.
A design may permit that representation while ensuring that no cooperating
reader accepts it as a completed account state. Its invariant is about
accepted snapshots, not every internal machine instruction. Conversely, if
negative inventory may trigger some independent action, saying that a later
rollback repairs the count may be too weak: the forbidden intermediate state
was already observable to the actor that mattered.

### A discussion that explains the course's method

**Reader, condensed:** Are we studying several unrelated techniques, or one
underlying problem?

**Explanation:** The techniques supply different kinds of evidence. A mutex
can establish exclusive access to shared state. A version check can establish
that the state did not change during a particular read. A quorum can establish
that enough distinct participants endorsed a decision. Each conclusion is
conditional on a different set of assumptions.

An assumption is not an embarrassment to hide after describing a guarantee.
It tells the reader where the guarantee comes from. The counter mutex relies
on all writers using the same lock. The versioned snapshot relies on writers
advancing the version correctly. A three-vote certificate relies on distinct
replica identities and appropriate voting rules. Replacing a cooperative
writer with an arbitrary faulty actor changes the model; it does not make
the earlier proof mysteriously wrong.

A failure trace is useful because it connects a broken assumption to a
specific forbidden transition. “There is a race” names a class of defect.
“A and B both read one remaining seat before either claims it” explains this
one. We will repeatedly turn that sentence into a small table, then move the
synchronization boundary until the forbidden trace is excluded.

### Reconstruct the argument

For any lab, write three sentences before running it: the result that would
count as a failure; the rule that supposedly excludes that failure; and the
participant or environmental behavior on which the rule depends. Then ask
which of those sentences the output actually checks. This is the habit that
makes the later move from threads to independent machines possible.

## 2. Lab: the counter and the lock that was not shared

Two workers each increment a counter one million times. After both finish,
the intended result is two million.

A source-level `counter++` looks like one action, but a useful conceptual
breakdown is read, compute, then write. Both workers can read the same old
value and overwrite one another's result.

| Step | Worker A | Worker B |
| --- | --- | --- |
| Read | Sees 0 | Sees 0 |
| Compute | Computes 1 | Computes 1 |
| Write | Stores 1 | Stores 1 |

This is an explanatory failure trace, not a complete specification of the
broken C program. Ordinary unsynchronized conflicting accesses create a
**C data race**, which causes undefined behavior. C does not promise merely
a smaller final count.

A broader **race condition** is a timing-dependent logical failure; one can
exist even when all individual accesses use atomics. A nondeterministic
winner, by itself, need not be a bug.

### The initial program — intentionally undefined

**Edited source listing — `counter_racy.c`:** compact formatting and an
explicit unused-argument annotation; the deliberate data race remains.

```c
#include <pthread.h>
#include <stdio.h>

#ifndef ITERATIONS
#define ITERATIONS 1000000L
#endif

long counter = 0;

void *worker(void *arg) {
    (void)arg;
    for (long i = 0; i < ITERATIONS; i++) {
        counter++;
    }
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("expected=%ld actual=%ld\n", 2L * ITERATIONS, counter);
    return 0;
}
```

Compile this particular demonstration with `-O0` first:

```bash
gcc -std=c11 -O0 -Wall -Wextra -pthread counter_racy.c -o counter_racy
./counter_racy
```

A wrong total exposes a symptom. A correct total does not establish safety.
Changing optimization may dramatically alter the symptom because the
program already has undefined behavior. `volatile` is not a repair.

### The first attempted repair: one private mutex per worker

The learner correctly suspected the lock's scope when their mutex version
still failed. A mutex declared inside `worker` is a different object in each
invocation. Both workers can own their respective locks simultaneously.

**Deliberately broken reconstruction — `counter_private_mutex.c`:**

```c
#include <pthread.h>
#include <stdio.h>

#ifndef ITERATIONS
#define ITERATIONS 1000000L
#endif

long counter = 0;

void *worker(void *arg) {
    (void)arg;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // Different object per worker
    for (long i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&lock);
        counter++; // Still a data race: workers use different locks
        pthread_mutex_unlock(&lock);
    }
    pthread_mutex_destroy(&lock);
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("expected=%ld actual=%ld\n", 2L * ITERATIONS, counter);
    return 0;
}
```

The shared resource is the counter, but there is no common coordination
object. Each worker has protected itself from itself. This example still
has undefined behavior; the presence of mutex calls is not enough.

### Correct repair A: one shared mutex

**Edited source listing — `counter_mutex.c`:**

```c
#include <pthread.h>
#include <stdio.h>

#ifndef ITERATIONS
#define ITERATIONS 1000000L
#endif

long counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg) {
    (void)arg;
    for (long i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("expected=%ld actual=%ld\n", 2L * ITERATIONS, counter);
    pthread_mutex_destroy(&lock);
    return 0;
}
```

This does not require the counter itself to be atomic. All concurrent
accesses to it occur under the same mutex, and main reads it after both joins.
The mutex supplies mutual exclusion and the necessary synchronization.

### Correct repair B: an atomic read-modify-write

**Consolidated source listing — `counter_atomic.c`:**

```c
#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>

#ifndef ITERATIONS
#define ITERATIONS 1000000L
#endif

atomic_long counter = 0;

void *worker(void *arg) {
    (void)arg;
    for (long i = 0; i < ITERATIONS; i++) {
        atomic_fetch_add(&counter, 1);
    }
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("expected=%ld actual=%ld\n", 2L * ITERATIONS, atomic_load(&counter));
    return 0;
}
```

`atomic_fetch_add` makes the increment an indivisible read-modify-write on
that object. It does not lock an arbitrary surrounding region of code.

**What changed?** The mutex version protects an agreed critical section;
the atomic version gives the particular increment its required semantics.
Both can preserve the same final-count property.

**Try:** identify which object is shared in each listing. Explain why moving
a mutex declaration changes correctness while changing a variable's cache-line
placement, by itself, does not repair this race.

### Why the private-mutex repair looks convincing

The attempted worker visibly contains `lock`, `counter++`, and `unlock`.
A reader following source indentation can therefore feel that the increment
is enclosed by protection. But mutex ownership belongs to an object, not to
a source-code location. Two calls to the same function can execute identical
source while operating on different stack objects.

Name the objects explicitly. Worker A creates `lock_A`; worker B creates
`lock_B`. A acquiring `lock_A` says nothing about whether B owns `lock_B`.
There is no conflict in the locking mechanism, so both increments can overlap.
The repair is to share the coordination object because the protected resource
is shared. A function-local `static` lock could also be shared; “must be a
global variable” is not the fundamental rule. The requirement is common
identity, suitable lifetime, and consistent participation.

**Reader, condensed:** I nearly put the lock beside the counter originally.
Does that mean a mutex and an atomic are doing the same thing underneath?

**Explanation:** They can preserve the same abstract increment while offering
different interfaces. A mutex says who may execute a region. An atomic
read-modify-write defines one object operation. Lock implementations commonly
use atomic primitives, but a call to `atomic_fetch_add` is not a request to
acquire a pthread mutex around whatever comes next.

### Separate the language defect from the algorithmic defect

The familiar read/add/write trace explains lost updates, but the ordinary
racy C program has a stronger problem: its accesses are not ordered as C
requires. It would be misleading to teach the smaller count as its only
possible result. Even choosing `-O0` does not restore a language guarantee;
it merely makes some implementation behavior easier to inspect.

We can isolate the algorithmic defect with a second experiment that uses
only atomic accesses. Each worker atomically loads zero. A barrier makes
both loads happen before either worker's store. Both then atomically store
one. Every individual access is defined and sequentially consistent, yet two
logical increments leave the counter at one.

**Additional controlled experiment — `counter_atomic_split.c`:**

```c
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

atomic_int counter = 0;
pthread_barrier_t after_load;

void *worker(void *arg) {
    (void)arg;
    int observed = atomic_load(&counter);
    pthread_barrier_wait(&after_load);
    atomic_store(&counter, observed + 1);
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_barrier_init(&after_load, NULL, 2);
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("logical increments=2 final counter=%d\n", atomic_load(&counter));
    assert(atomic_load(&counter) == 1);
    pthread_barrier_destroy(&after_load);
    return 0;
}
```

The barrier is a test instrument. It does not corrupt the counter or weaken
the atomics. It arranges an execution that was already possible for a split
read/compute/write algorithm. Replacing the split sequence with
`atomic_fetch_add` changes the algorithmic boundary and gives the result two.

This gives us a three-way classification worth retaining throughout the book:

| Program | Individual conflicting accesses | Larger decision |
| --- | --- | --- |
| Ordinary unprotected counter | C data race | Incorrectly decomposed increment |
| Atomic load plus atomic store | Defined atomic accesses | Still loses a logical update |
| Atomic fetch-add | Defined atomic read-modify-write | Correct increment operation |

The output alone cannot tell these categories apart. The source and its
synchronization argument do. This is why a later replica can be data-race-free
while still returning two incompatible approvals.

### Check your understanding

If A and B use two different mutexes, may both lock calls succeed? Yes. If
both use the same mutex, does that choose A as the winner? No. If each
increment is atomic, does a separate update to `transactions` become part of
that atomic operation? No. These are three different questions about identity,
ordering, and scope.

## 3. Lab: compare-and-swap and the unexpected winner

CAS stands for **compare-and-swap**; the C API calls it compare-and-exchange.
It makes the comparison and conditional replacement one atomic operation.

For strong CAS, the conceptual rule is:

| Comparison | Effect on object | Return | Effect on `expected` |
| --- | --- | --- | --- |
| Object equals expected | Replaced with desired | Success | Retains the matching expected value |
| Object differs | Unchanged by this attempt | Failure | Updated with the observed object value |

Let two workers try to change a slot from 0 to their own nonzero ID.

**Edited source listing — `cas_claim.c`:**

```c
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

atomic_int slot = 0;

void *worker(void *arg) {
    int id = *(int *)arg;
    int expected = 0;
    if (atomic_compare_exchange_strong(&slot, &expected, id))
        printf("Thread %d CLAIMED the slot\n", id);
    else
        printf("Thread %d LOST; observed owner %d\n", id, expected);
    return NULL;
}

int main(void) {
    pthread_t a, b;
    int id_a = 1, id_b = 2;
    pthread_create(&a, NULL, worker, &id_a);
    pthread_create(&b, NULL, worker, &id_b);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("Final slot owner: %d\n", atomic_load(&slot));
    return 0;
}
```

Exactly one successful claim is possible in this one-shot example. Once one
worker replaces 0, the other's comparison against 0 fails. The loser learns
the owner observed by the failed comparison.

### “How can thread 2 ever get the slot?”

Creating thread A first does not require A to reach the claim first. Thread
creation order is not execution order. A may be ready while B runs; A may
also run and be paused before the CAS.

The guarantee is one owner, not a predetermined owner. Scheduler
nondeterminism is compatible with correctness.

The source experimented with delays to make another winner easier to see.
An empty delay loop may disappear under optimization. A sleep or extra
logging changes the experiment's schedule; neither is a proof of an ordering
constraint unless the program uses an appropriate synchronization mechanism.

### Weak and strong do not mean weaker and stronger correctness

Strong CAS does not spuriously fail when its comparison matches. Weak CAS
may fail spuriously, so it is normally used where the algorithm already
retries. Failure still reports an observed value through `expected`.

A lock-acquisition loop must restore the expected unlocked value on every
attempt:

```c
for (;;) {
    int expected = 0;
    if (atomic_compare_exchange_weak(&locked, &expected, 1))
        break;
}
```

If a failed attempt changes `expected` to 1 and the next attempt uses that
value without resetting it, a `1 → 1` success could be mistaken for acquiring
an already-owned lock. The surrounding protocol matters as much as the
primitive.

On the x86 examples discussed, weak and strong could compile to the same
`lock cmpxchg` instruction. On other implementations the compiler may need
to hide hardware retries for strong CAS. The API contract is portable; a
particular instruction mapping is an implementation detail.

### The assembly connection

For an illustrative x86 `cmpxchg`, the accumulator supplies the expected
value, another operand supplies the desired replacement, and the flags
report whether the comparison succeeded. `sete` can turn that flag into a
Boolean. A `lock` prefix on the memory read-modify-write supplies the relevant
cross-core atomicity; it is not a pthread mutex or a command to stop every
other processor.

Inspect your actual compiler's output:

```bash
gcc -std=c11 -O2 -pthread -S -masm=intel cas_claim.c -o cas_claim.s
```

**Try:** draw the two valid winning histories, then explain why both
successful `0 → ID` transitions cannot occur without an intervening reset.

### Reading a failed CAS as useful information

Before the call, `expected` is a proposed premise: “I believe the slot is
zero.” On success, the operation has made the conditional transition from
zero to this worker's ID. On failure, the comparison supplies a different
observed value. The failure is the mechanism telling the caller that its
premise was not sufficient to authorize the update.

Imagine A wins with ID 1. B's call does not first pass a check and later get
interrupted before replacement. The check and conditional replacement are
the indivisible part. B compares against the already occupied slot, receives
failure, and learns an observed owner of 1. This is why moving a plain `if`
next to an assignment cannot reproduce CAS semantics.

The value reported through `expected` is still an observation made by that
operation. In a general mutable system it may change again immediately
thereafter. CAS does not freeze the object after returning. The slot lab is
especially easy to prove because the slot is never reset: once nonzero,
it stays nonzero. That monotonic rule removes many complications that will
reappear in reusable locks and versioned structures.

### The scheduling experiment inside the uploaded file

The supplied `cas_claim.c` contains a `spinner()` call intended to add jitter.
That is a valuable experimental idea, but the particular function has several
independent C problems. It is used before a matching declaration; its defined
signature takes an argument while the call supplies none; and its random
arithmetic can overflow signed integers. Its loop result is unused, so an
optimizer may remove the intended work. Reseeding shared pseudo-random state
from each worker also makes the experiment harder to interpret.

The teaching listing above omits that incidental mechanism so readers can
reason about the claim itself. The submitted version is preserved in
`originals/`. A properly specified sleep can bias arrival order without
changing the single-owner rule. If one run shows worker 2 winning, that is
not the atomic operation becoming less reliable. It is evidence that our
assumption about which thread runs first was unjustified.

### Expected value is part of the algorithm

For a one-shot ticket claim, a failure can mean “this ticket was taken; try
another ticket.” For an increment loop, it can mean “recalculate the desired
value from the value just observed.” For lock acquisition, it means “try
again with the expected *unlocked* value.” Blindly reusing the failure value
is correct for some algorithms and incorrect for others.

That difference is easy to miss because all three use the same function.
The primitive does not know whether the caller is acquiring a lock, updating
inventory, or publishing a replacement configuration. The surrounding state
machine gives success and failure their meanings.

**Pause and predict:** if a slot can later return from 1 to 0, can two claims
over the lifetime of the program succeed? Of course; they could refer to
different ownership intervals. Does equality with a value prove nothing
happened in between? No. That question introduces the ABA problem, which
was named in the roadmap but not implemented in this progression.

### One small file, three atomic operations

The uploaded `big_atomic_energy.c` is deliberately an assembly probe rather
than an executable: it has no `main`. Build it as an object or assembly file.
The functions illustrate fetch-add, unconditional exchange, and conditional
compare-exchange. In this compact teaching copy only formatting changes.

```c
#include <stdatomic.h>

atomic_int x = 0;

void add(void) {
    atomic_fetch_add(&x, 1);
}

void exchange(void) {
    atomic_exchange(&x, 42);
}

int cas(void) {
    int expected = 42;
    return atomic_compare_exchange_strong(&x, &expected, 100);
}
```

```bash
gcc -std=c11 -O2 -c big_atomic_energy.c -o big_atomic_energy.o
gcc -std=c11 -O2 -S -masm=intel big_atomic_energy.c -o big_atomic_energy.s
```

An unused returned old value can affect which instruction sequence a compiler
chooses. Inspect the generated program rather than treating one mnemonic
as the definition of the C operation. The language contract is the part on
which the source-level correctness argument rests.

## 4. Lab: building mutual exclusion from an atomic primitive

The next realization was that atomics and mutexes are not unrelated choices.
An atomic operation can establish who owns a lock; the ownership protocol
then protects a much larger region of ordinary code.

C's `atomic_flag_test_and_set` returns the previous flag value while setting
it to true. Observing false means this caller acquired an unowned flag.
Observing true means it must keep waiting.

**Edited source listing — `spin_counter.c`:**

```c
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#ifndef ITERATIONS
#define ITERATIONS 1000000L
#endif

long counter = 0;
atomic_flag lock = ATOMIC_FLAG_INIT;

void spin_lock(void) {
    while (atomic_flag_test_and_set(&lock)) {
        // Busy wait
    }
}

void spin_unlock(void) {
    atomic_flag_clear(&lock);
}

void *worker(void *arg) {
    (void)arg;
    for (long i = 0; i < ITERATIONS; i++) {
        spin_lock();
        counter++;
        spin_unlock();
    }
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("expected=%ld actual=%ld\n", 2L * ITERATIONS, counter);
    return 0;
}
```

The flag operations here use their default sequentially consistent ordering.
They provide the ordering needed for the protected ordinary counter; a
relaxed flag-only replacement would not establish the same protection for
surrounding non-atomic data.

### “How would the spinlock be done using a mutex?”

The spinlock is itself a lock implementation. A pthread mutex is another
mechanism for establishing mutually exclusive access. You would normally
replace the homemade spinlock with a mutex rather than wrap one in the other.

| Mechanism | What a losing participant does | Scope of the guarantee |
| --- | --- | --- |
| Atomic increment | Completes its own increment according to atomic semantics | That object operation |
| CAS claim | Receives failure and follows the algorithm's policy | Conditional object transition |
| Simple spinlock | Actively retries lock acquisition | Critical section followed by all participants |
| Pthread mutex | May block under contention; implementation may also spin | Critical section followed by all participants |

Making the protected region longer can preserve the final count while
wasting much more CPU on spinning. A lock owner that stops progressing can
leave waiters stuck. This simple spinlock promises neither fairness nor a
bounded waiting time.

**The useful hierarchy:** an atomic ownership transition supports a lock
protocol, which protects a critical section and the invariant maintained
inside it. An ordinary `long` is safe because of that protocol, not because
its source-level increment became inherently atomic.

### The flag is not protecting the counter by association

Declaring an atomic flag near an ordinary counter does not create protection.
The causal argument is the actual protocol: only the caller whose atomic
operation observes a clear flag may enter; all other callers remain outside;
the owner clears the flag only after finishing the protected work.

Now add visibility. A handoff must do more than prevent two owners from
being inside at once. The new owner must also have the ordering needed to
use changes made by the previous owner. The default atomic operations in
the listing supply that ordering. A more explicit conventional form uses
acquire for successful test-and-set and release for clear:

```c
while (atomic_flag_test_and_set_explicit(&lock, memory_order_acquire)) {
}
/* Protected ordinary accesses. */
atomic_flag_clear_explicit(&lock, memory_order_release);
```

We will unpack acquire/release in the publication chapter. Here the essential
point is that reducing these to relaxed operations would change the proof
for the ordinary counter, even if the flag itself remained atomic.

### Busy work, blocked work, and progress

Suppose the owner spends 100 milliseconds inside the critical section. A
simple spin waiter keeps executing atomic attempts during that interval.
Those attempts do not help finish the owner's work. On a heavily loaded
machine they may consume a CPU that some useful worker could have used.
A mutex implementation may instead arrange for a waiting thread to sleep,
although it may also spin briefly or have other fast paths.

This is not a categorical claim that mutexes are slow and spinlocks are fast.
There are several costs: uncontended acquisition, expected waiting duration,
scheduler transitions, available cores, cache traffic, and the owner's chance
to run. A short critical section on an otherwise idle machine is a different
workload from a lock held across a blocking operation.

A stopped lock owner also exposes a progress distinction. Other workers may
keep taking steps but none can enter. An algorithm built using atomic
instructions is therefore not automatically a *lock-free algorithm*. The
progress property concerns the whole algorithm, not the name of an operation
inside it. Our spinlock is a deliberately simple lock, with no fairness or
owner-failure recovery guarantee.

### A particularly useful comparison

The ordinary counter under the spinlock is safe for the same structural
reason as the ordinary counter under the mutex: conflicting accesses are
ordered by a common protocol. Its type did not become a magic exception to
C's rules. If a diagnostic thread reads it without taking the lock while
workers are writing, that extra participant can break the protocol.

Readers often focus only on writers. But reading an ordinary object while
another thread modifies it also requires the relevant synchronization.
“Read-only monitoring” is still an access to the shared state. The same
lesson will matter when a database dashboard or a replica coordinator tries
to inspect a state transition from outside its intended observation boundary.

## 5. Lab: correct programs can still fight over a cache line

The next experiment moved below C variables into physical memory behavior.
Two threads can update different atomic objects correctly while contending
on the same cache-coherence unit.

A cache line is a block tracked by the cache/coherence machinery. The course
used an assumed 64-byte line environment. Sixty-four is a hardware assumption
for this experiment, not a rule of C.

Very roughly, a coherence protocol lets multiple cores retain readable
copies but coordinates ownership for conflicting writes. MESI names four
states: Modified, Exclusive, Shared, Invalid. These are a useful model;
real implementations include additional machinery and variations.

The important boundary mismatch is:

| C algorithm sees | Hardware may see |
| --- | --- |
| Worker A owns counter A | Both counters occupy one cache line |
| Worker B owns counter B | Both writers repeatedly need that line |

This is **false sharing**: logically independent writes contend because of
layout. Updating the exact same counter is true sharing. Neither label
implies that atomic operations stop being correct.

**Edited source benchmark — `false_sharing.c`:** preserves the three cases,
uses compact calls, adds result/layout checks, and defaults to fewer iterations.
The original display code subtracted pointers to distinct subobjects; this
listing reports standard `offsetof` values instead of relying on that pointer
subtraction.

```c
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#ifndef ITERATIONS
#define ITERATIONS 10000000L
#endif
#define CACHE_LINE 64

_Alignas(CACHE_LINE) atomic_long shared_counter = 0;
struct Together {
    _Alignas(CACHE_LINE) atomic_long a;
    atomic_long b;
} together;
struct Apart {
    _Alignas(CACHE_LINE) atomic_long a;
    _Alignas(CACHE_LINE) atomic_long b;
} apart;

_Static_assert(offsetof(struct Together, b) + sizeof(atomic_long) <= CACHE_LINE, "Together must fit within 64 bytes");
_Static_assert(offsetof(struct Apart, b) >= CACHE_LINE, "Apart needs separate assumed lines");

void *worker(void *arg) {
    atomic_long *counter = arg;
    for (long i = 0; i < ITERATIONS; i++)
        atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
    return NULL;
}

double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void run(const char *label, atomic_long *a, atomic_long *b) {
    pthread_t ta, tb;
    atomic_store(a, 0);
    if (a != b) atomic_store(b, 0);
    double start = now();
    pthread_create(&ta, NULL, worker, a);
    pthread_create(&tb, NULL, worker, b);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    double elapsed = now() - start;
    long ca = atomic_load(a), cb = atomic_load(b);
    assert(ca == (a == b ? 2L * ITERATIONS : ITERATIONS));
    assert(cb == (a == b ? 2L * ITERATIONS : ITERATIONS));
    printf("%s: A=%ld B=%ld seconds=%.6f addresses=%p,%p\n", label, ca, cb, elapsed, (void *)a, (void *)b);
}

int main(void) {
    printf("member offsets: together=%zu apart=%zu assumed_line=%d\n", offsetof(struct Together, b), offsetof(struct Apart, b), CACHE_LINE);
    run("same atomic", &shared_counter, &shared_counter);
    run("false sharing", &together.a, &together.b);
    run("separate lines", &apart.a, &apart.b);
    return 0;
}
```

All three cases must produce the correct counts. The false-sharing and
separate-line cases perform the same kind of per-object atomic increment.
Their arrangement changes the expected coherence traffic.

`_Alignas(64)` requests a 64-byte boundary. The structure layout and static
checks establish the intended relative placement for this build. They do
not query the running processor's cache-line size.

### What the experiment can establish

A large timing difference is consistent with the cache-contention mechanism,
but one timing run is not a direct trace of coherence states. Core placement,
shared versus separate physical cores, optimization, frequency changes,
other workloads, and thread startup affect the result. Affinity or supported
hardware performance counters can make a more controlled follow-up.

Putting `printf` in the hot loop can dominate the operation being studied.
The library may synchronize internally, while I/O perturbs scheduling and
cache state. Print results after the measured work when investigating the
increment cost. The source's surprising per-thread progress output was a
lesson about instrumentation as well as scheduling.

### Why not make every coherence unit tiny?

Cache-line size is an engineering tradeoff. Tracking larger blocks can
reduce metadata and exploit spatial locality; it also couples unrelated
nearby writes. False sharing exposes where the nearby-access assumption
fails for this workload. Padding every object has its own memory costs.

**Try:** confirm the reported offsets, then compare counts and time as
separate questions. Explain why moving the counters changes performance
without changing the correctness argument.

### Why two separate variables can behave like one hot resource

A useful first model of the benchmark is to imagine the cores negotiating
permission to write a block. Updating `a` needs permission for the block
containing `a`. Updating adjacent `b` may need permission for that exact same
block. The application has separate logical owners, but the hardware has a
coarser ownership unit.

This makes the separate-line result intelligible without assuming the CPU
understands the program's worker IDs. Padding changes the placement so that
the two workers no longer repeatedly demand the same assumed coherence
unit. The atomic operations still do the same kind of indivisible updates.
The improvement comes from reducing interference beneath the language's
object boundary.

### What the exploratory `cache_battle.c` actually measures

The uploaded exploratory file does more than increment two counters. Each
worker also prints the other counter on every iteration. That means the
workload includes cross-reading, stream-library synchronization, formatting,
and enormous output. It is a different experiment from two independent
writers whose only interference is cache-line placement.

There is also a source-level trap in `main`: local `pthread_t a, b` names
hide the global atomic counters named `a` and `b`. The final print statements
therefore refer to thread handles rather than the intended counters. A
pthread handle is an opaque API type; printing it as though it were an
ordinary signed long is not a portable way to inspect it. The missing
`stdio.h` declaration is a separate compile-time issue.

These are not reasons to discard the exploratory file. They are exactly why
experiments need a narrow claim. One version can be useful for observing
interleaving and logging. Another version should isolate counts and memory
layout before drawing timing conclusions. The controlled teaching benchmark
is the latter.

### Why relaxed increments are sufficient here

Each worker's obligation is to perform its numeric updates atomically. There
is no associated ordinary payload that another worker is supposed to start
using when the count changes. We read final results after joining the workers.
The program therefore does not need those increments to publish unrelated
state. Relaxed describes ordering requirements, not a request for approximate
arithmetic or weaker atomicity.

This is an important contrast with a `ready` flag. If an observer treats the
flag as permission to use a separate ordinary buffer, its proof needs an
ordering edge for that buffer. The same memory-order argument cannot be
copied from a numeric counter simply because both variables are atomic.

### Interpreting the printed addresses

The addresses and member offsets are useful evidence about this build's
layout. They do not reveal the full physical cache topology. For threads in
one process, the deliberately aligned structure is enough to construct the
intended common 64-byte-line experiment under its stated hardware assumption.
For independent processes, comparing virtual pointer values would not prove
that they refer to the same physical line.

**A good explanation of a fast run has two parts:** the counts satisfy the
logical requirement, and the layout reduces a plausible performance cost.
Neither statement should be used as a substitute for the other.

## 6. Lab: a semaphore baton and a finish barrier

The learner then asked for a stronger rule: A increments, then B, then A,
then B. A mutex permits one owner at a time, but generally does not choose
which waiting thread becomes the next owner.

Two semaphores can encode permission to proceed. A starts with one permit;
B starts with zero. Each worker consumes its permit, touches the ordinary
counter, then posts the other worker's permit.

**Consolidated source listing — `thread_relay.c`:** uses the corrected barrier
API, compact formatting, and retries `sem_wait` when interrupted by a signal.

```c
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#define TURNS 10
long counter = 0;
sem_t turn_a, turn_b;
pthread_barrier_t finish_line;

void wait_turn(sem_t *turn) {
    while (sem_wait(turn) == -1) {
        if (errno == EINTR) continue;
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
}

void *worker_a(void *arg) {
    (void)arg;
    for (int i = 0; i < TURNS; i++) {
        wait_turn(&turn_a);
        printf("A: %ld\n", ++counter);
        sem_post(&turn_b);
    }
    puts("A reached finish barrier");
    pthread_barrier_wait(&finish_line);
    puts("A exits");
    return NULL;
}

void *worker_b(void *arg) {
    (void)arg;
    for (int i = 0; i < TURNS; i++) {
        wait_turn(&turn_b);
        printf("B: %ld\n", ++counter);
        sem_post(&turn_a);
    }
    puts("B reached finish barrier");
    pthread_barrier_wait(&finish_line);
    puts("B exits");
    return NULL;
}

int main(void) {
    pthread_t a, b;
    sem_init(&turn_a, 0, 1);
    sem_init(&turn_b, 0, 0);
    pthread_barrier_init(&finish_line, NULL, 2);
    pthread_create(&a, NULL, worker_a, NULL);
    pthread_create(&b, NULL, worker_b, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("Final counter: %ld\n", counter);
    sem_destroy(&turn_a);
    sem_destroy(&turn_b);
    pthread_barrier_destroy(&finish_line);
    return 0;
}
```

The increment lines alternate `A: 1`, `B: 2`, through `B: 20`. The ordinary
counter is safe because the handoff orders its accesses. No atomic counter
or additional mutex is necessary for this protocol.

The finish barrier establishes that neither worker proceeds past that phase
until both have arrived. It does not make their next CPU instructions execute
at the same instant, and it does not specify the order of their exit messages.
After A posts B's last permit, B may even reach the finish barrier before A
prints its own arrival message.

### The compiler error was an API distinction

An earlier attempted listing used `pthread_cond_wait` with a barrier object.
The correction was `pthread_barrier_wait(&finish_line)`. A condition variable
is a different mechanism; its wait operation also needs an associated mutex.
The mismatch did not indicate a flaw in semaphore handoff.

| Primitive | Rule expressed |
| --- | --- |
| Atomic operation | This particular object transition is indivisible |
| Mutex | One participating owner may execute the protected region |
| Semaphore | A count of permits/events controls progress |
| Two-semaphore baton | Permission is handed to a particular next participant |
| Barrier | A phase cannot finish until all required participants arrive |

These mechanisms constrain legal histories. They do not prescribe a complete
scheduler timeline. If a participant never reaches a required barrier, the
others can wait indefinitely; correct participant counts and reachable phases
are part of the assumptions.

**Try:** explain why replacing the baton with an ordinary shared counter
and two unconstrained loops loses the alternation guarantee even if the
counter itself becomes atomic.

### Trace the permits rather than the CPU scheduler

Initially A has permission and B does not. Suppose the scheduler runs B first.
B reaches `sem_wait` and waits; it does not increment first merely because it
received CPU time first. When A eventually runs, it consumes its permit,
increments, and posts B's semaphore. B can then continue.

After A posts B, A may run again immediately. But A's next attempt to consume
its own semaphore cannot succeed until B posts it. This is the stronger rule
that a single mutex would not provide. A mutex prevents overlapping critical
sections but may allow A to acquire repeatedly. The two-semaphore protocol
encodes *which participant* becomes eligible next.

| Moment | A permit | B permit | Who can next increment? |
| --- | --- | --- | --- |
| Initial | 1 | 0 | A |
| A consumes permit | 0 | 0 | A is already inside its turn |
| A finishes and posts B | 0 | 1 | B |
| B consumes and later posts A | 1 | 0 | A |

The table describes the protocol, not necessarily values that can be sampled
simultaneously from outside it. It is a reasoning model for how permission
moves.

### The ordinary counter has an ordered ownership chain

The critical fact is not that the variable is at one stable address. That
was the confusion that later made the ready-flag example look contradictory.
A stable address can still be read and written incorrectly. The semaphore
handoff supplies the synchronization that makes the ordinary accesses valid.
The storage location tells us *which object* is being accessed; the handoff
establishes *when access is permitted*.

The last turn also explains the barrier's role. A posts B's final permit
before A reaches the finish barrier. B can therefore finish and arrive at
the barrier first. Both arrival messages must precede both exit messages,
but there is no required ordering between the two arrivals or the two exits.
“Both released” is a statement about eligibility, not simultaneous physical
execution.

### Semaphore permits and condition-variable notifications differ

A semaphore carries a count. Posting before a later wait makes a permit
available. A condition variable is not a persistent queue of permission
tokens. Its normal pattern is a shared predicate under a mutex, a loop that
checks the predicate, and a wait that atomically releases the mutex while
blocking, then reacquires it before returning.

A notification asks a waiter to check the predicate again. It does not prove
that the predicate is now true, nor does a notification sent with no waiter
become a future permit. This distinction will explain a failed scheduling
experiment in the replica chapter. We will not try to reproduce semaphore
behavior by scattering an unprotected Boolean and `cond_signal` through
unrelated critical sections.

**Predict first:** if both semaphores begin at zero, what makes the first
increment possible? Nothing. If both begin at one, what forbids both workers
from touching the ordinary counter together? The required baton invariant
has disappeared. Initialization is part of the synchronization algorithm.

## 7. Why memory and isolation have several different boundaries

The cache example prompted a broader question: if C code can reach ordinary
memory so directly, what makes process, container, and VM boundaries real?

The answer requires distinguishing the units that different mechanisms track.

| Layer or mechanism | Relevant unit or responsibility |
| --- | --- |
| C object model | Object lifetime, type, bounds, and valid operations |
| Allocator | Allocated/free chunks and allocator metadata |
| Cache coherence | Cache lines and cross-core visibility/ownership |
| Virtual memory | Page mappings and access permissions |
| Kernel | Processes, credentials, resources, namespaces, and policy |
| Virtualization | Guest execution and guest-to-host memory/resource boundaries |

`malloc` metadata does not normally cause the processor to check an object's
allocation bounds on every pointer dereference. An invalid C access can be
undefined behavior while landing in a mapped page that produces no immediate
hardware fault. A crash in `free` may expose corruption that occurred earlier.

Page protection is different from allocator bookkeeping. Likewise, coherence
maintains the required consistency of accesses; it does not decide whether
application code was authorized to issue a pointer access.

### Virtual addresses are not physical identity

Two processes can use the same numerical virtual address while referring
to different physical memory. Merely matching virtual addresses does not
create shared data or cache-line contention on that data. Conversely,
explicitly shared mappings can expose the same physical storage through
different virtual addresses.

Threads in a process share its address space. An application's intended
per-thread ownership of two adjacent fields is not a hardware protection
boundary, which is why false sharing is possible in the earlier lab.

### Containers and VMs change the enforcement assumptions

The discussion connected this to the **trusted computing base**: components
whose correct behavior a guarantee depends on.

Containers commonly rely on a shared host kernel to maintain isolation
through permissions, namespaces, resource controls, and related mechanisms.
A VM adds a guest boundary, including virtualization-controlled translation
between guest physical and host physical memory on the model discussed.

Compromising a guest kernel does not, by itself, grant control of the
hypervisor's mappings. Nor does a VM provide a literal physical wall:
virtualization code, device interfaces, and lower-level hardware remain part
of the relevant assumptions.

The conceptual comparison from the supplied video discussion is retained;
this edition does not reproduce or independently evaluate the linked video's
product recommendations. The lesson is the enforcement question:

> Which lower-level mechanism makes the claimed higher-level boundary real?

A performance symptom, a memory-safety violation, and an isolation escape
are different outcomes. A common underlying habit is to identify the actual
boundary rather than infer one from a convenient software label.

### The same byte participates in several different systems

Imagine an allocation holding two fields. The allocator may regard it as
one live chunk. C regards each field as an object with language rules about
valid access. The page tables make the page readable or writable. The cache
tracks a line containing some of those bytes. These mechanisms are not
competing descriptions; they answer different questions about the same
storage.

This is why an out-of-bounds write can evade an immediate hardware fault.
The write may land on a mapped writable page while violating the language's
object bounds and damaging another allocation. The MMU is not normally
consulting `malloc`'s allocation metadata to discover that the pointer crossed
an application object boundary. The mistake remains a C defect even if the
processor completes the access.

The converse is equally useful. Correct C logic operating through an invalid
mapping can fault. A correct language-level access protocol does not grant
permission to read arbitrary physical memory. The layers' assumptions have
to fit together.

### A shared cache is not the same as shared writable address space

The source's question about unrelated programs “touching the same 64 bytes”
needed one refinement. Matching virtual addresses in separate processes can
map to different physical pages. The numerical pointer alone does not
identify a shared physical cache line. Shared mappings, by contrast, can
create genuine sharing even when the processes use different virtual
addresses.

Also distinguish direct access from indirect interference. Two workloads can
compete for a shared hardware resource without either receiving permission
to dereference the other's memory. Performance interference and side-channel
questions concern what can be inferred from shared resources; an ordinary
unauthorized direct read is a different mechanism. Neither should be waved
away merely because a process abstraction exists.

### Why the container and VM connection mattered

The original discussion became more coherent once “container” and “VM” were
replaced by the mechanism doing the enforcement. A container boundary commonly
depends on the host kernel enforcing a collection of policies. A guest VM
also has a kernel, but its view of physical memory is itself constrained by
a lower virtualization boundary.

That does not mean a guest kernel is unimportant or the lower layer is
infallible. It means a particular guest compromise does not automatically
include authority over the hypervisor's mappings. The next questions are
which interfaces cross that boundary and which components must be trusted
for the intended isolation claim.

This is the same question we asked about the counter: who is participating
in the protection protocol, and what can bypass it? The mechanism is vastly
different, but the form of reasoning is familiar. We are no longer satisfied
with “the bytes belong to this object” or “the process is inside a container”
as a complete causal explanation of safety.

## 8. Ownership, lifetime, and the camera/MEX case study

The thread revisited an older camera-integration problem to apply this way
of thinking to a real system. Raw frame storage remained in native code;
a separate MATLAB output array was returned. The discussion explicitly
corrected an earlier interpretation that MATLAB had received the raw buffer.

The remembered observations included a crash when freeing one allocation,
continued operation with a leak when the free was removed, and changed
behavior when the allocation routine changed. The original reproducer and
full historical conditions were unavailable. The cause was not established.

An allocator substitution changing a symptom does not prove the allocator
was broken. Zero initialization, an overrun, alignment, retained SDK access,
or earlier heap corruption are different hypotheses requiring evidence.
Omitting a free and avoiding a crash does not prove the remaining program
is safe; it removes a reclamation step and leaks resources.

### Reconstruct the ownership contract

| Object | Questions to establish |
| --- | --- |
| Raw frame buffer | Who allocates it, which SDK/device actors can access it, and when does that access end? |
| MATLAB output | Is it a separate allocation with a separate lifetime? |
| Camera/system handle | Which close or release operation invalidates it? |
| Configuration/node map | In which acquisition states may it be modified? |

A possible ownership model to verify against the actual API is:

| Stage | Transition whose contract must be established |
| --- | --- |
| Allocated | Storage exists |
| Registered | Another subsystem may retain access |
| Acquiring | Work may still use the buffer |
| Complete and unregistered | Required users have relinquished access |
| Freeable | Reclamation is authorized |
| Freed | Any further access is invalid |

This is an illustrative contract, not a reconstruction of an unavailable
vendor implementation. The important forbidden transition is reclamation
while any other actor still has permission to access the object.

A function returning or local processing finishing does not necessarily prove
that a callback, worker, driver, or device is finished. Conversely, hidden
participants must be established from evidence rather than assumed to exist.

The same issue appears with immutable snapshots: constructing a replacement
is not permission to free an older version that a reader still holds.
Immutability simplifies changes to an object, but does not by itself solve
its publication or lifetime.

**Try:** write a table of allocator, writers/readers, owner, valid-until event,
and deallocator before adding locks. A mutex cannot repair a fundamentally
incorrect lifetime contract by itself.

### “I finished using it” is a local statement

A buffer can pass through a function whose visible work is complete while
another actor still retains a reference. Examples include an asynchronous
SDK operation, a callback queue, or a device transfer. The relevant completion
event is whatever the actual API promises, not the end of a convenient local
block of code.

That was the useful shape of the historical camera discussion. The learner
clarified that raw acquisition storage was not handed directly to MATLAB.
The output array and the acquisition buffer had separate roles. Once that
clarification was made, an explanation based simply on MATLAB retaining the
raw pointer no longer fit the described architecture.

This correction demonstrates an important debugging discipline: a plausible
story must be revised when ownership facts change. We should not preserve
the first theory merely because it uses vocabulary such as DMA, caching,
or use-after-free. Those words name mechanisms that need evidence.

### Why changing allocation can change a symptom

Suppose an allocator replacement changes initial bytes or the placement of
an object. A program that accidentally reads an uninitialized field could
behave differently. So could a program with a size error, a retained pointer,
or heap corruption exposed during reclamation. Several different defects
can respond to the same experimental change.

The observations “freeing crashes,” “not freeing leaks but avoids this
crash,” and “another allocation pattern changes the outcome” do not uniquely
identify a cause. In particular, they do not establish that freeing a valid,
unreferenced allocation through its correct deallocator is generically
unreliable. The diagnosis remained unresolved in the source, and remains
unresolved here.

A useful investigation would enumerate the participants, then find the
contractual event at which each loses access. If an SDK says a frame-complete
callback guarantees no more access, that is a specific candidate boundary.
If it merely says a request was queued, it is not the same evidence. The
actual API documentation or a reproducer must resolve which case applies.

### Two independent questions about a handle

A valid lifetime does not imply every operation is currently allowed. A
camera handle can still exist while configuration changes are forbidden
during acquisition. Conversely, an operation that would be legal for an idle
camera cannot use a handle that has already been released.

Represent those separately: lifetime establishes whether the object exists;
state establishes which transitions are permitted now. Synchronization must
then establish that the state relied upon cannot change in the forbidden
interval between check and use.

This yields a practical exercise that does not require the old hardware.
Write a hypothetical `IDLE → CONFIGURING` claim as one guarded transition.
Explain why a bare `if (state == IDLE)` followed later by a configuration
call leaves an interval in which another actor can start acquisition. You
have rediscovered TOCTOU without needing a security exploit.

## 9. Lab: publishing ordinary data with acquire/release

The baton already proved an ordering. The next question was whether an atomic
ready flag can publish an ordinary payload without a semaphore or mutex.

An atomic flag with only relaxed accesses makes accesses to that flag
well-defined. It does not establish the required cross-thread ordering for
an ordinary `int data`:

```c
/* Deliberately broken publication pattern: data is non-atomic. */
data = 42;
atomic_store_explicit(&ready, 1, memory_order_relaxed);

/* Another thread: no synchronizes-with edge for data. */
while (atomic_load_explicit(&ready, memory_order_relaxed) == 0) {
}
printf("%d\n", data);
```

This pattern can have a C data race on `data`. It is not a portable experiment
that promises merely an occasional old value, even if it repeatedly prints
42 on the machine tested.

**Edited source listing — `publication.c`:** the corrected one-shot version.
It was introduced and explained in the original thread; this edition's new
validation run is separate evidence, not a claim that the learner demonstrated
both outcomes there.

```c
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

int data = 0;
atomic_int ready = 0;

void *producer(void *arg) {
    (void)arg;
    data = 42;
    atomic_store_explicit(&ready, 1, memory_order_release);
    return NULL;
}

void *consumer(void *arg) {
    (void)arg;
    while (atomic_load_explicit(&ready, memory_order_acquire) == 0) {
    }
    printf("data = %d\n", data);
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, producer, NULL);
    pthread_create(&b, NULL, consumer, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    return 0;
}
```

When the acquire load observes the release store's value, that store
synchronizes with the load. The ordinary payload write before the store
therefore happens-before the payload read after the load.

| Event | Reason for its order |
| --- | --- |
| Producer writes `data` | Sequenced before publishing `ready` |
| Producer release-stores 1 | Publication event |
| Consumer acquire-load observes that 1 | Synchronizes with the release publication |
| Consumer reads `data` | Sequenced after the observing load |

The data can remain an ordinary integer because its conflicting accesses
are now ordered. The payload is not modified again in this one-shot program.
Repeated publication needs additional rules so a writer cannot overwrite a
payload while a reader still uses it; the one-shot example does not solve
that broader problem.

### Memory ordering is not memory layout

The question that clarified this was why the baton counter could be a plain
`long`, while the new ready flag needed an atomic type.

The baton has synchronization around the counter's accesses. The ready flag
is itself part of the synchronization mechanism and is directly polled by
one thread while another updates it. The acquire/release flag then orders
access to a different, ordinary payload.

| Concept | Concern |
| --- | --- |
| Layout | Where bytes and objects are placed; alignment and cache-line sharing |
| Ordering | Which relationships between operations the language model establishes |
| Lifetime | Whether the object still exists and access remains valid |

Relaxed atomics are useful when only that atomic object's value matters,
as in a numeric count without a payload-publication requirement. The default
sequentially consistent operations used in earlier labs provide stronger
ordering, but still do not make several distinct operations one transaction.

The source also noted that acquire/release and relaxed loads or stores can
compile to similar instructions on x86. Identical-looking instructions do
not erase the C-level compiler constraints. Inspect actual output; do not
infer portable guarantees from one architecture's behavior.

### Follow the exact permission carried by `ready`

The producer first writes the payload and then release-stores 1 into `ready`.
The consumer is allowed to use the payload only after an acquire load
observes that publication. Merely executing an acquire operation somewhere
in the program does not import every write ever made by every thread.
The observation connecting these two operations is essential.

In this one-shot program, zero means “do not use the payload yet,” and one
means the publication has occurred. The producer does not modify the payload
after publishing it. The consumer does not read it speculatively and keep
that read if the flag later changes. These small state-machine facts are
what make the example easy to prove.

**Reader, condensed:** Why was an ordinary `long` safe in the relay, but
now the flag needs to be atomic? Did changing its memory location matter?

**Explanation:** The relay has semaphores supplying the synchronization.
The flag has no enclosing semaphore or mutex: one thread polls it while
another updates it, so its accesses need their own defined coordination.
That atomic flag can then supply an ordering edge for an ordinary payload.
Layout did not answer either question.

### A sentence that should be used carefully

“Acquire makes the data visible” is useful shorthand, but can conceal the
proof. The fuller statement is: the producer's payload write is sequenced
before its release; the consumer's acquire observes that release; the
consumer's subsequent payload read is therefore ordered after the producer's
write. That is the relationship the C model provides.

No physical stopwatch measurement substitutes for that relationship. A
producer may appear to finish before a consumer starts using a buffer in
several tests, while the program lacks the synchronization required by the
language. Similarly, a relaxed flag may appear to work on a particular
compiler and CPU without establishing a portable contract for the ordinary
payload.

### Why the repeated-use version is a different problem

Suppose the producer publishes 42, then immediately writes 43 into the same
ordinary payload for the next message. The consumer might still be reading
the first publication. One acquire observation does not grant exclusive
ownership of a perpetually reused buffer.

A reusable design needs another rule: an acknowledgement, a ring-buffer
ownership protocol, a lock, independent immutable versions, or some other
mechanism that prevents overwriting a payload still in use. Those are design
alternatives to investigate, not consequences of changing a memory-order
keyword in the one-shot example.

**Check:** would making only `data` atomic solve the publication interpretation?
It would define that object's individual accesses, but the question of which
message/version the consumer is accepting would still need a protocol.
Atomic types can remove one class of undefined behavior without completing
the algorithm.

## 10. Lab: atomic inventory operations, non-atomic decisions

The original thread ended by offering a limited-inventory example: eight
workers make attempts to reserve 100 tickets. The essential safety property
is that inventory never falls below zero.

### Version A: atomic load, then atomic decrement — logically broken

**Edited source listing — `inventory_naive.c`:**

```c
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define THREADS 8
#define TICKETS 100
#define ATTEMPTS 100000

atomic_int remaining = TICKETS;

void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < ATTEMPTS; i++) {
        if (atomic_load_explicit(&remaining, memory_order_relaxed) > 0) {
            atomic_fetch_sub_explicit(&remaining, 1, memory_order_relaxed);
        }
    }

    return NULL;
}

int main(void) {
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, worker, NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Remaining: %d\n", atomic_load(&remaining));
    return 0;
}
```

At the last ticket, A and B can both load 1. A decrements to 0; B then
atomically decrements to -1. No individual atomic operation was violated.
The combined check-and-change was not protected.

A particular run can finish at 0 because it did not encounter that schedule.
This source program does not force the rare window. The later one-seat lab
provides a deliberately arranged, well-defined logical failure.

### Version B: protect the whole decision with a mutex

**Edited source listing — `inventory_mutex.c`:**

```c
#include <pthread.h>
#include <stdio.h>

#define THREADS 8
#define TICKETS 100
#define ATTEMPTS 100000

int remaining = TICKETS;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < ATTEMPTS; i++) {
        pthread_mutex_lock(&lock);

        if (remaining > 0)
            remaining--;

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main(void) {
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, worker, NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Remaining: %d\n", remaining);
    return 0;
}
```

The ordinary integer is sufficient because both the availability check and
its decrement occur under one shared mutex. Main reads it after joining all
workers. With successful thread execution and enough attempts, the final
inventory is zero and never negative.

### Version C: validate and decrement with CAS

**Edited source listing — `inventory_cas.c`:**

```c
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define THREADS 8
#define TICKETS 100
#define ATTEMPTS 100000

atomic_int remaining = TICKETS;

int try_buy_ticket(void) {
    int current = atomic_load_explicit(&remaining, memory_order_relaxed);

    while (current > 0) {
        if (atomic_compare_exchange_weak_explicit(&remaining, &current, current - 1, memory_order_relaxed, memory_order_relaxed)) {
            return 1;
        }
    }

    return 0;
}

void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < ATTEMPTS; i++)
        try_buy_ticket();

    return NULL;
}

int main(void) {
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, worker, NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Remaining: %d\n", atomic_load(&remaining));
    return 0;
}
```

A successful CAS commits `current → current - 1` only if that positive
`current` still matches. On failure, `current` is refreshed with the observed
value, and the loop reevaluates availability. Spurious weak-CAS failures are
also retried. The complete program still relies on progress of participating
threads; it does not promise a bounded number of retries.

### The bridge into the continuation

The original source proposed adding `sold` and requiring an accounting
relationship such as `remaining + sold == TICKETS`. This needs an explicit
observation boundary. Even under a mutex, the two machine-level assignments
occur separately; cooperating readers taking the same mutex can observe the
complete logical operation. Independent unlocked reads do not inherit that
transactional view.

The continuation revisited these same issues through immutable publication,
database updates, a one-seat reservation, and multi-field snapshots. We now
have the foundation needed to follow that progression without treating each
new synchronization API as an isolated trick.

### Draw the permission gap at the last ticket

The critical input is not how many workers exist overall. It is that at
least two can make a decision from the same positive observation.

| Event | Inventory | A's local observation | B's local observation |
| --- | --- | --- | --- |
| A loads | 1 | 1 | — |
| B loads | 1 | 1 | 1 |
| A decrements | 0 | 1 | 1 |
| B decrements | -1 | 1 | 1 |

Both decrements are perfectly atomic. The algorithm lacks an operation that
means “decrement only while the inventory still satisfies the decision's
premise.” Sequentially consistent loads and decrements would still permit
this trace. Blaming `memory_order_relaxed` would misidentify the problem.

A mutex puts both the current-value check and decrement inside one region.
The CAS loop instead proposes a transition based on the observed count and
commits only if that count still matches. When it fails, it revisits the
premise using the value reported by the failed comparison.

### What does a retry actually repeat?

If the first observed value is 10, a worker proposes 9. If another worker
wins first, the local `current` may become 9. The next attempt proposes 8.
It must not keep trying to store the old desired value 9 after changing its
expected value to 9, because that would no longer describe buying a ticket.
The desired value is derived again from the current premise.

If failure updates `current` to zero, the loop exits without issuing a
successful reservation. If a weak comparison fails spuriously while the
value remains positive, the loop can try again. There is no need to interpret
every failed attempt as another customer sale or a corrupt state.

### Tempting repairs and the property they actually offer

One possible idea is to decrement unconditionally, inspect the returned old
value, and add one back if there was no stock. Such a design can sometimes
be made to count successful claims under a carefully chosen contract. It
does not automatically preserve “the shared count is never negative at any
observable point.” Other code could inspect the negative intermediate value,
perform another action, or make a decision before compensation occurs.

That is why the property must precede the proposed repair. A final corrected
number is a different promise from preventing invalid intermediate states
from becoming observable to other participants. We need not reject every
compensating design; we must state the actual semantics it implements.

**Exercise:** replace “tickets” with “coupon not yet redeemed.” What must be
inside the guarded transition? The test for redemption and the change that
claims it. A separate later email or charge also needs its own relationship
to the successful claim, which a local CAS does not establish by itself.

## 11. Immutability moves coordination to publication

Suppose many threads read a configuration with two related fields:

```c
struct Config {
    int max_connections;
    int timeout_ms;
};
```

A mutable design can protect reads and updates with a common mutex. A writer
updates both fields while holding the lock; readers take that same lock while
copying the fields they need. This works when the entire relevant operation
is inside the agreed boundary.

An alternative is to construct a new version privately, then publish one
reference to it:

```c
_Atomic(struct Config *) current;

/* Writer: initialize every field of next before publishing it. */
atomic_store_explicit(&current, next, memory_order_release);

/* Reader: load once, then use that one immutable version. */
struct Config *cfg = atomic_load_explicit(&current, memory_order_acquire);
int limit = cfg->max_connections;
int timeout = cfg->timeout_ms;
```

This is a pattern fragment, not a complete program: initialization,
allocation, and reclamation are additional responsibilities. An acquire load
that observes the release publication makes the preceding initialization
available to the reader. The published object must then remain immutable.

### The question that clarified the comparison

*Does the globally scoped mutex approach fail?*

No. Both designs can be correct. A global mutex works only when every
relevant reader and writer uses it and holds it for the necessary scope.
Immutable publication changes where the coordination cost occurs.

| Design | Reader work | Writer work | Remaining challenge |
| --- | --- | --- | --- |
| Mutable object under a mutex | Acquire lock and copy/read state | Update inside the lock | Contention and critical-section scope |
| Immutable versions | Load a reference and read one version | Build and publish a new version | Publication, conflicting writers, and object lifetime |

Read-heavy workloads can benefit from avoiding a lock operation on every
read. Copying large objects for frequent updates can instead be expensive.
These are workload questions, not a ranking of primitives.

### Two problems publication does not solve

First, the old version cannot be freed while a reader still uses it. A
writer's replacement of `current` does not revoke existing readers'
references. Reference counting, epochs, hazard pointers, and RCU were named
as related approaches, but their implementation was not completed here.

Second, two writers may both derive replacements from the same old version.
If both use unconditional stores, one writer's changes can disappear. When
updates depend on prior state, the publication step may need a writer lock
or compare-and-swap with recomputation after conflict. An atomic reference
store makes replacement indivisible; it does not merge independent changes.

**Takeaway:** immutable objects simplify shared reading, while coordination
remains at publication and reclamation boundaries.

### One pointer can select a large coherent object

The advantage of immutable publication is not that the new configuration has
only one field. It is that one chosen reference identifies a version whose
fields will not change under the reader. The reader loads the reference
once and uses that same version for the operation.

If it loads `current` separately for each field, it can accidentally select
one field from version A and another from version B. The underlying versions
are individually immutable, but the reader has assembled a result spanning
two versions. “The objects never change” does not excuse a poorly defined
snapshot boundary.

This is another granularity choice. What belongs in one version? If
`timeout_ms` and `max_connections` must be interpreted together, selecting
one shared configuration object gives that relationship a clear boundary.
If the entire application is copied for every tiny change, copying may be
too expensive. Structural sharing or smaller versioned components can reduce
copying, but relationships between components then need explicit treatment.

### The difficult work has moved, not disappeared

**Reader, condensed:** So the atomic publication still needs ownership to
change when two writers publish. Is the benefit simply that the conflict is
much smaller?

**Explanation:** Yes. Readers mostly perform shared loads of immutable
storage, while writers contend over selecting the current version. But two
writers deriving changes from the same old version still need to preserve
their intended update semantics.

| Step | Writer 1 | Writer 2 |
| --- | --- | --- |
| Read | Selects A | Selects A |
| Work privately | Builds B with one change | Builds C with another change |
| Publish | Stores B | Stores C |

Unconditional pointer stores permit C to replace B, losing writer 1's
change if both changes were supposed to accumulate. If the contract is
“replace the whole configuration with this independent value,” that may be
acceptable. If the contract is “apply my change to the current configuration,”
the writer needs conflict validation or serialization.

A pointer CAS can validate the assumed base version. On failure, rebuilding
from the newly observed version may preserve the other writer's change.
However, a full implementation also needs to keep referenced versions alive
while writers and readers use them. Do not free a failed or retired base
object simply because one CAS returned; other participants may hold it.

### `const` is a discipline, not a reclamation system

A `const`-qualified access path helps express an intention not to mutate
through that path. It does not provide lifetime management or stop every
other alias from making an otherwise invalid modification. The safe design
includes the publication protocol, the promise that readers see immutable
versions, and a reclamation rule.

This is why immutability connected so strongly to the camera case: both
require proof that every relevant participant has finished before storage is
reclaimed. The publication point can be tiny even when the selected state
is large; the lifetime obligation still spans all participants using it.

## 12. Database transactions and stale decisions

The same shape appears when application code reads a database value,
calculates outside the database, and later writes a replacement.

Start with a balance of 100. Two clients read it. One computes a withdrawal
of 20 and plans to write 80. The other computes a deposit of 50 and plans
to write 150.

| Step | Client A | Client B |
| --- | --- | --- |
| Observe | Reads 100 | Reads 100 |
| Calculate privately | Computes 80 | Computes 150 |
| Update | Writes literal 80 | Writes literal 150 |

Serializing the final writes alone still permits a final balance of 80 or
150, depending on order. Neither represents applying both intended changes;
that result would be 130.

A lock on the final assignment does not protect an earlier assumption.
This is time-of-check/time-of-use reasoning: the state was valid when
observed, but may have changed before the decision commits.

### Express the operation inside its protection boundary

For the single-row arithmetic example discussed in the thread, putting the
operation in the database allows it to use the relevant current row:

```sql
UPDATE accounts SET balance = balance - 20 WHERE id = 1;
UPDATE accounts SET balance = balance + 50 WHERE id = 1;
```

Conditional updates can express the invariant directly:

```sql
UPDATE inventory
SET quantity = quantity - 1
WHERE product_id = 42 AND quantity > 0;
```

The caller checks whether the update affected a row. A prior successful read
is not evidence that this later conditional update succeeded.

Two other patterns discussed were holding a row lock across a transaction's
read/decision/write sequence, and validating a version:

```sql
UPDATE accounts
SET balance = 80, version = version + 1
WHERE id = 1 AND version = 42;
```

Zero affected rows means the expected version was not matched. The
application must handle that outcome, commonly by rereading and retrying.
These SQL fragments illustrate the conversation's single-row examples;
complete transaction design must account for its actual isolation semantics
and every row or predicate involved.

### Versions do not automatically reconcile writers

MVCC gives readers access to appropriate row versions. That does not mean
two writers can independently modify the same row and have their business
intent merged afterward. Writer conflict handling is a separate requirement.
Similarly, serializable execution means committed behavior corresponds to a
valid serial history; it does not require literally executing all work one
transaction at a time.

The thread contrasted versioned readers with a single-owner execution model:
multiple clients can submit work while one executor serializes mutations.
Both approaches relocate coordination. Neither removes the need to define
valid transitions.

### Provisional results and committed results

The discussion also connected this to the Polymarket “Ghost Fills” example:
one component can report a provisional outcome before another component has
committed the underlying transition. The general lesson is to preserve the
distinction between *observed*, *validated at an earlier time*, and
*committed*. This course uses that architectural connection without
reproducing or independently verifying the external incident claims.

**Takeaway:** protect the relationship between observation, decision, and
commit, rather than only the final write.

### The database did not misunderstand the two assignments

If the application asks the database to set the balance to 80 and later to
150, serializing those assignments can correctly produce 150. The database
has not been told that 80 meant “subtract 20 from whichever balance is
current” or that 150 meant “add 50.” That business intent existed only in
the clients' earlier private calculations.

This was the exact point of surprise in the discussion: a storage system
can protect its internal data structures and execute each requested update
correctly while the application's overall operation loses an intended change.
The missing information is the dependency of each replacement on the old
balance.

### Follow both possible orders

For operation expressions, the simple single-row example has two histories:

| Order | First operation | Second operation | Final balance |
| --- | --- | --- | --- |
| Withdrawal then deposit | 100 − 20 = 80 | 80 + 50 = 130 | 130 |
| Deposit then withdrawal | 100 + 50 = 150 | 150 − 20 = 130 | 130 |

For stale literal replacements, the histories are different:

| Order | First replacement | Second replacement | Final balance |
| --- | --- | --- | --- |
| A then B | Set 80 | Set 150 | 150 |
| B then A | Set 150 | Set 80 | 80 |

The arithmetic happens to commute in the first example. Other operations may
not commute, so “both orders have the same answer” is not the general
definition of correctness. The general requirement is the ordering and
transactional behavior promised for the operations being submitted.

PostgreSQL's documented Read Committed behavior for a conflicting update to
a predetermined row includes waiting for the first updater and, if it commits,
checking the updated row before applying the later operation. This supports
the simple example. It is not a promise that every multi-row application
invariant is automatically preserved at that isolation level.

### Express a condition where it can be checked at commit

A version predicate makes the dependency explicit: “this replacement was
computed from version 42; only apply it if that version still matches.”
Checking the affected-row count is part of that protocol. Ignoring a zero-row
result and telling the customer the operation succeeded would undo the
benefit of the validation mechanism at the application level.

Alternatively, a transaction can lock the relevant row before reading and
keep the protection through its decision and write. The useful interval
starts with the observation on which the decision depends, not merely the
last assignment. More involved predicates or multiple rows require a
transaction design appropriate to those relationships.

**Reader, condensed:** So a concurrency-safe database cannot make arbitrary
application code safe automatically?

**Explanation:** Correct. It provides operations with defined semantics.
The application must express its intended invariant using those operations,
constraints, and transaction boundaries. That is the same lesson as the
private mutex and early unlock, at a different abstraction layer.

## 13. Lab: one seat, two buyers

There is one seat. Two buyers attempt to reserve it. At the end:

- At most one ticket may have been issued.
- Successful sales and remaining seats must account for the original seat.

The source's first unprotected C program used ordinary shared integers. It
therefore had C data races as well as the intended stale-decision bug. Its
output cannot be promised under the C language rules.

The edited lab below uses atomics in all three modes to isolate the logical
bug. In naive mode, both buyers read 1 before a barrier releases them. Both
then store 0 and issue a ticket. Atomic reads and stores make these accesses
well-defined without making the whole reservation correct.

**Edited teaching version — `seats.c`:**

```c
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

enum { NAIVE, MUTEX, CAS };
atomic_int seats = 1, sold = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t gate;
int mode;

void *reserve(void *arg) {
    const char *name = arg;
    if (mode == MUTEX) {
        pthread_barrier_wait(&gate); // Rendezvous before taking the lock
        pthread_mutex_lock(&lock);
        if (atomic_load(&seats) > 0) {
            atomic_fetch_sub(&seats, 1);
            atomic_fetch_add(&sold, 1);
            printf("%s bought a ticket\n", name);
        }
        pthread_mutex_unlock(&lock);
    } else {
        int expected = atomic_load(&seats);
        pthread_barrier_wait(&gate); // Both observed the original seat count
        if (expected > 0) {
            if (mode == NAIVE) {
                atomic_store(&seats, expected - 1);
                atomic_fetch_add(&sold, 1);
                printf("%s bought a ticket from a stale snapshot\n", name);
            } else if (atomic_compare_exchange_strong(&seats, &expected, expected - 1)) {
                atomic_fetch_add(&sold, 1);
                printf("%s bought the ticket\n", name);
            } else {
                printf("%s lost the race; seats now %d\n", name, expected);
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "naive") && strcmp(argv[1], "mutex") && strcmp(argv[1], "cas"))) {
        fprintf(stderr, "usage: %s naive|mutex|cas\n", argv[0]);
        return 2;
    }
    mode = !strcmp(argv[1], "naive") ? NAIVE : !strcmp(argv[1], "mutex") ? MUTEX : CAS;
    pthread_t a, b;
    pthread_barrier_init(&gate, NULL, 2);
    pthread_create(&a, NULL, reserve, "A");
    pthread_create(&b, NULL, reserve, "B");
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("remaining=%d sold=%d\n", atomic_load(&seats), atomic_load(&sold));
    pthread_barrier_destroy(&gate);
    pthread_mutex_destroy(&lock);
    return 0;
}
```

Run each mode:

```bash
./seats naive
./seats mutex
./seats cas
```

| Mode | Final remaining seats | Final tickets sold | Mechanism |
| --- | --- | --- | --- |
| Naive | 0 | 2 | Both commit from the same stale snapshot |
| Mutex | 0 | 1 | Check and change share one critical section |
| CAS | 0 | 1 | Only one expected-value comparison can claim the seat |

These totals were reproduced in the new validation run. Buyer names and
print order may differ.

### Why both correct versions work

The mutex version prevents another buyer from intervening between checking
availability and recording the sale. The CAS version permits simultaneous
reasoning but conditions the claim on the state still matching the expected
value. On CAS failure, C updates `expected` with the value observed by the
comparison.

This example uses strong CAS so failure does not include a spurious failure
with an unchanged value. A weak-CAS loop must also account for that possibility.

### A barrier is a rendezvous, not a reservation rule

The barrier makes an interesting schedule easy to observe. It does not decide
who owns the seat. In the mutex branch it is before lock acquisition. In the
other branches it is after both snapshots have been read.

Putting a two-thread barrier inside a mutex that both participants must
acquire can deadlock: one thread holds the mutex and waits for the other,
while the other cannot reach the barrier because it needs that mutex.

### A boundary worth noticing

The successful CAS reserves the seat; the sale counter is incremented in a
separate operation. After both threads finish, the accounting is correct.
An arbitrary observer reading the two counters independently does not get a
transactional snapshot. The invariant's observation boundary matters.

**Try:** explain the exact instant at which a reservation becomes committed
in each correct mode. Then explain why a plausible final `seats == 0` does
not establish that the naive version sold only one ticket.

### Why the two correct reservation programs looked so similar

The learner expected “optimistic” to allow something resembling the first
unprotected program. The name seemed to suggest accepting a risk of
incoherence for speed. The experiment corrected that interpretation.

Both buyers may read the same available seat. The optimistic design is
prepared for that. What it refuses to do is let both callers turn that
observation into a successful claim. The validation gate is the essential
part of optimism, not an optional cleanup stage.

The pessimistic design pays by excluding overlap in the protected decision.
The optimistic design permits overlap in the reasoning, then one attempted
commit fails. Neither is permitted to issue a second valid ticket for the
same seat under this contract. The possible customer-visible outcome is
still one success and one failure; the mechanism and wasted work differ.

### Follow the exact contents of the local variables

A and B both load `expected = 1`. Each computes `desired = 0`. Suppose A's
strong CAS succeeds. B's later comparison sees zero instead of one, leaves
the seat at zero, and overwrites B's `expected` with that observed zero.
B's printed failure is positive evidence that the conflicting claim was
rejected. It is not evidence that the system briefly issued a bad ticket.

In the mutex version, B might never get to reason from a seat count of one
inside the lock: it acquires after A and sees zero immediately. That avoids
this particular speculative failure by making B wait earlier. Waiting is
the cost that does not appear in the optimistic “lost races” counter.

### The plain original and the controlled teaching version

The uploaded `reservations.c` forces both snapshots before the writes, but
uses ordinary shared integers for the conflicting writes. It has a C data
race in addition to the logical overselling structure. Treat its plausible
printed result as a symptom, not a portable deterministic contract.

The combined teaching `seats.c` uses atomic accesses even in its deliberately
wrong mode. That lets the book promise and check the logical failure without
asking undefined C behavior to serve as a reliable experiment. This edit
preserves the lesson while making its observation much more precise.

The uploaded pessimistic example initializes a barrier it does not use.
That barrier contributes no protection. The mutex is sufficient for the
check-and-update operation. The optimistic example uses its barrier to expose
the common snapshot; CAS is the mechanism preserving the single-claim rule.
Removing a test barrier changes how often a failure attempt occurs, not the
core correctness argument.

**Check:** are “customer B did not obtain this seat” and “a bad ticket was
issued to B” interchangeable? No. A failed acquisition can be exactly the
correct response to contention.

## 14. Lab: many uniquely numbered tickets

One seat hides much of the performance distinction because every buyer
competes for the same resource. Giving tickets independent IDs allows us to
ask whether unrelated reservations need to coordinate.

The source compared a global table mutex with one atomic owner per ticket.
Ticket IDs are array indices. Owner `-1` means free; other values identify
the worker that claimed the ticket.

**Edited source program — `ticket_compare.c`:** the algorithm is preserved;
post-run ownership/count assertions have been added, and the CAS call has
been compacted.

```c
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define TICKETS 100000
#define THREADS 8

enum { PESSIMISTIC, OPTIMISTIC };

_Atomic int owner[TICKETS];
_Atomic int sold = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct Stats {
    int id;
    int mode;
    long attempts;
    long occupied;
    long races;
    long wins;
};

uint32_t rng(uint32_t *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

void *worker(void *arg) {
    struct Stats *s = arg;
    uint32_t seed = 0x9e3779b9u ^ (s->id + 1);

    while (atomic_load_explicit(&sold, memory_order_relaxed) < TICKETS) {
        int ticket = rng(&seed) % TICKETS;
        s->attempts++;

        if (s->mode == PESSIMISTIC) {
            pthread_mutex_lock(&lock);

            if (atomic_load_explicit(&owner[ticket], memory_order_relaxed) == -1) {
                atomic_store_explicit(&owner[ticket], s->id, memory_order_relaxed);
                atomic_fetch_add_explicit(&sold, 1, memory_order_relaxed);
                s->wins++;
            } else {
                s->occupied++;
            }

            pthread_mutex_unlock(&lock);
        } else {
            if (atomic_load_explicit(&owner[ticket], memory_order_relaxed) != -1) {
                s->occupied++;
                continue;
            }

            int expected = -1;

            if (atomic_compare_exchange_strong_explicit(&owner[ticket], &expected, s->id, memory_order_relaxed, memory_order_relaxed)) {

                atomic_fetch_add_explicit(&sold, 1, memory_order_relaxed);
                s->wins++;
            } else {
                s->races++;
            }
        }
    }

    return NULL;
}

double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void run_test(int mode) {
    pthread_t threads[THREADS];
    struct Stats stats[THREADS];

    atomic_store(&sold, 0);

    for (int i = 0; i < TICKETS; i++)
        atomic_store(&owner[i], -1);

    double start = now();

    for (int i = 0; i < THREADS; i++) {
        stats[i] = (struct Stats){
            .id = i,
            .mode = mode
        };

        pthread_create(&threads[i], NULL, worker, &stats[i]);
    }

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    double end = now();
    int claimed = 0;
    for (int i = 0; i < TICKETS; i++) {
        int id = atomic_load(&owner[i]);
        assert(id >= 0 && id < THREADS);
        claimed++;
    }
    assert(claimed == TICKETS && atomic_load(&sold) == TICKETS);

    long attempts = 0;
    long occupied = 0;
    long races = 0;
    long wins = 0;

    for (int i = 0; i < THREADS; i++) {
        attempts += stats[i].attempts;
        occupied += stats[i].occupied;
        races += stats[i].races;
        wins += stats[i].wins;
    }

    assert(wins == TICKETS);
    printf("\n%s\n", mode == PESSIMISTIC ? "PESSIMISTIC" : "OPTIMISTIC");
    printf("tickets sold: %ld\n", wins);
    printf("attempts:     %ld\n", attempts);
    printf("occupied:     %ld\n", occupied);
    printf("lost races:   %ld\n", races);
    printf("time:         %.6f s\n", end - start);
}

int main(void) {
    run_test(PESSIMISTIC);
    run_test(OPTIMISTIC);
}
```

### What the counters measure

| Counter | Meaning |
| --- | --- |
| `wins` | Successful claims by this worker |
| `occupied` | Chosen ticket was already owned when checked |
| `races` | Ticket looked free, but the later strong CAS did not acquire it |
| `attempts` | Total randomly chosen ticket IDs examined |

The crucial clarification was: **a lost race is a failed attempt to acquire
a valid ticket, not an invalid ticket being issued.** Both implementations
must preserve the ownership rule. Optimism concerns whether speculative work
will survive validation, not whether correctness is optional.

The pessimistic version prevents a competing claim within the protected
operation. The optimistic version allows the overlap, then rejects a stale
claim. Both can be correct while paying different coordination costs.

### How to read the benchmark honestly

This experiment compares more than mutex instructions against CAS
instructions. It also compares one global lock with per-ticket ownership.
A per-ticket mutex design would be another legitimate comparison.

Both branches use atomic storage in this shared harness. Atomics inside the
mutex are not needed merely to protect that ticket's ownership when every
access follows the same lock protocol. The shared representation makes the
comparison convenient but contributes overhead of its own.

Other limits are visible in the program:

- Every successful sale updates the shared `sold` counter.
- Adjacent owners and adjacent per-worker statistics can share cache lines.
- Random selection wastes more attempts near sellout because most tickets
  are already occupied.
- The tests run sequentially; scheduling, cache state, and machine conditions
  affect timings.
- Relaxed ordering is sufficient here for numeric ownership and counting;
  it is not a publication mechanism for unrelated ticket payload fields.

Do not infer that CAS is always faster. In the new validation run, each mode
sold 100,000 tickets; the optimistic mode recorded six lost races. That is
one observation, not a prediction for another machine or thread count.

**Try:** vary the thread count, then explain changes in attempts, occupied
selections, races, and elapsed time separately. A high retry count is not by
itself evidence of invalid results.

### Four numbers, four different stories

A run might report a million attempts, almost a million occupied selections,
100,000 wins, and only a handful of lost races. That is not contradictory.
An *occupied selection* notices a ticket already owned when it first checks.
A *lost race* first observes a free ticket, then loses the conditional claim
before its strong CAS commits. Most failed searches near sellout can belong
to the first category without representing close simultaneous claims.

A mutex version has no comparable failed commit inside the protected
check-and-set, but it can spend time waiting to reach that check. Reporting
zero lost races does not mean it paid zero coordination cost. Conversely,
a larger number of attempts does not mean it made more useful progress.

### Why random searching gets expensive near the end

As a reasoning model, assume each selection independently chooses uniformly
among N ticket IDs. After k tickets are taken, the chance that the next
selection finds a free ticket is `(N-k)/N`. The expected number of selections
until the next success is therefore `N/(N-k)` in that model.

The early successes are easy. The last free ticket has probability `1/N` on
each selection, so finding it takes N selections on average under those
assumptions. Summing the stages gives `N * (1 + 1/2 + ... + 1/N)` expected
selections to collect all tickets. This is an explanation of the search
cost, not a performance prediction for our deterministic PRNG and real
thread interleaving.

It tells us what to separate experimentally. Millions of occupied selections
can arise from the workload itself, not just from the synchronization
primitive. Replacing random search with a work-allocation strategy would
change the experiment as well as its runtime.

### The benchmark compares coordination granularity too

The global mutex serializes A claiming ticket 23 with B claiming ticket 81,
even though the IDs are independent. Per-ticket CAS does not impose that
same global exclusion. A per-ticket mutex would be another design worth
comparing before concluding the entire difference is “mutex versus atomic.”

Both paths still update the shared sold count. Both use per-worker statistics
whose adjacent storage can false-share. Both random streams and the loop's
termination check contribute work. These are not defects in a small learning
benchmark, but they limit the conclusion it can support.

**Reader, condensed:** Aren't both branches already using atomics? Does that
make the comparison misleading?

**Explanation:** It makes the implementation convenient and the claim narrower.
The important distinction is where the logical check-and-claim is coordinated.
Some atomic accesses in the locked branch are redundant for its own exclusion
protocol. A carefully controlled benchmark should state these choices rather
than advertise one universal winner.

**Exercise:** given `attempts=1000`, `wins=100`, `occupied=897`, `races=3`,
which attempts issued invalid tickets? None are implied. The counts describe
100 successes and 900 attempts that did not acquire a ticket. You still need
the ownership invariant and its implementation to establish that the successes
were valid.

## 15. Correct executions and state machines

The learner's turning point was recognizing that correctness applies to every
allowed execution, not merely the output observed in a few runs.

For one ticket, a valid transition is:

```text
FREE → owned by A
```

An attempt by B after that transition must fail or follow an explicitly
defined later ownership transition. It must not silently create a second
owner. The state machine and ownership model define what synchronization
has to preserve.

A mutex and CAS can implement the same abstract transition with different
execution strategies. One controls entry to the decision. The other makes
the commit conditional on the observed state still being current.

This also explains why a “whole database in memory” does not make either
strategy universally preferable. Short private work, expensive external
operations, read/write mix, conflict frequency, and retry cost all matter.
A CDN label by itself is not enough to select a synchronization policy.

For an optimistic path, repeated computation must be safe to discard.
Sending an email, charging a card, or writing to another independent system
before validation is not undone merely because a later CAS fails. Such
side effects need their own commitment and duplication rules.

**A reusable reasoning checklist:**

1. What state does the decision depend on?
2. Who may change that state?
3. What transition is allowed?
4. Where does the transition become accepted?
5. What invalidates an observation made earlier?
6. What happens after waiting, failed validation, or participant failure?

The next lab expands the relevant state from one ownership value to a
relationship between multiple fields.

### A valid execution is more than a plausible final value

Consider two workers that both report owning ticket 23 while the final owner
array contains only worker B. The final array looks like a valid single-owner
state. But A has already been told its claim succeeded. If success authorizes
something outside that array, overwriting the owner does not retract A's
permission. The accepted history, not just the final representation, matters.

That is why we identify a **linearization point** in these small examples:
a point at which the abstract operation can be regarded as taking effect.
For the CAS seat claim it is the successful conditional update. For the
mutex-protected claim, we can choose the state change inside the protected
operation because cooperating observers cannot interleave an incompatible
operation there. The implementation may span many instructions while the
abstract operation has one effect in the relevant history.

This is not a claim that every program using a mutex is automatically
linearizable. Its operation boundary, returns, and observer protocol still
need to fit the abstract behavior. An early return announcing success before
the guarded change would require a different argument.

### Work that can be thrown away, and work that cannot

Optimistic computation is easiest when it prepares a private result that can
be discarded. Calculate a candidate configuration, validate the base version,
then publish. If validation fails, the private result can be reconsidered.

An irreversible external effect is different. Suppose a worker charges a
customer before its ticket CAS. If the CAS fails, discarding a local variable
does not undo the charge. “Retry the transaction” may charge again. The system
needs a design connecting the external effect to commitment, including whatever
idempotency or deduplication its actual contract requires.

The course did not implement a general external transaction system, so we
should not imply that a short CAS loop solves one. It supplied the reasoning
habit: distinguish private speculative work, accepted local state, and effects
outside the mechanism's control.

### An ownership model can remove a local race

The later networking exercise will have many connections in progress but only
one thread processing their events. That can give one owner of local node
state. Multiple socket operations exist concurrently in time, yet local
handlers do not execute simultaneously. Some mutex problems disappear by
architecture rather than by adding a new lock.

The machines still run independently, however. A single-threaded node can
receive stale leadership information and make a locally coherent but globally
unsafe decision. Eliminating local data races does not establish a single
authoritative history across those machines.

The progression is therefore not “keep adding synchronization until nothing
can overlap.” It is to choose a state machine, identify its owners and
acceptance rules, then use the mechanism appropriate to that scope.

## 16. Lab: consistent snapshots of multiple fields

Start with checking 500 and savings 500. Transfer 100 from checking to
savings. Complete states must satisfy:

```text
checking + savings == 1000
```

| State | Checking | Savings | Total |
| --- | --- | --- | --- |
| Before transfer | 500 | 500 | 1000 |
| After first field update | 400 | 500 | 900 |
| Complete transfer | 400 | 600 | 1000 |

The intermediate 900 really can exist in memory during this implementation;
it is not a completed transfer state. A reader can also combine values from
different moments, such as old checking and new savings, and obtain 1100.
Individual atomic accesses do not make the pair one snapshot.

### Validate the version before accepting the snapshot

Use an even version for a stable state and an odd version while a writer
updates it. Writers serialize with a mutex. Readers accept their collected
fields only when the starting version is even and equals the ending version.

**Edited source program — `snapshot.c`:** `nanosleep` replaces `usleep` for
POSIX feature-macro compatibility; a straightforward retry loop replaces the
source's `do/while`; the retry counter is widened.

```c
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

atomic_int checking = 500, savings = 500;
atomic_uint version = 0;
pthread_mutex_t writer_lock = PTHREAD_MUTEX_INITIALIZER;

void delay_ms(long ms) {
    struct timespec t = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}

void *writer(void *arg) {
    (void)arg;
    pthread_mutex_lock(&writer_lock);
    atomic_fetch_add(&version, 1); // Odd: update in progress
    atomic_store(&checking, 400);
    delay_ms(100);
    atomic_store(&savings, 600);
    atomic_fetch_add(&version, 1); // Even: complete version
    pthread_mutex_unlock(&writer_lock);
    return NULL;
}

void *reader(void *arg) {
    (void)arg;
    unsigned long long retries = 0;
    for (;;) {
        unsigned before = atomic_load(&version);
        if (before & 1u) {
            retries++;
            continue;
        }
        int c = atomic_load(&checking);
        int s = atomic_load(&savings);
        unsigned after = atomic_load(&version);
        if (before != after) {
            retries++;
            continue;
        }
        printf("checking=%d savings=%d total=%d version=%u retries=%llu\n", c, s, c + s, after, retries);
        return NULL;
    }
}

int main(void) {
    pthread_t w, r;
    pthread_create(&w, NULL, writer, NULL);
    delay_ms(20); // Encourages overlap; does not guarantee scheduling
    pthread_create(&r, NULL, reader, NULL);
    pthread_join(w, NULL);
    pthread_join(r, NULL);
    pthread_mutex_destroy(&writer_lock);
    return 0;
}
```

All shared fields in this listing are atomic, and their operations use the
default sequentially consistent ordering. That is deliberately a simpler
model than an optimized production sequence-lock implementation.

Do not replace the fields with ordinary unsynchronized C integers and argue
that a later version mismatch makes the race harmless. Rejecting an
observation afterward does not undo undefined behavior from a C data race.
The earlier conceptual version-counter sketch is a reasoning aid, not
permission for non-atomic concurrent payload access.

### What is established, and under which assumptions?

A reader may accept the complete old state or the complete new state,
depending on scheduling. The delay encourages overlap; it is not a scheduling
guarantee. A new run accepted `(400, 600)` at version 2 and retried while the
writer was active.

The argument relies on cooperating writers, serialization of writers, correct
version updates, and no version wraparound between the reader's version
checks. The one-update demonstration avoids wraparound. A general reusable
implementation would need a bound or other treatment for it.

Readers can starve under sustained writing. A writer that stalls while the
version is odd can prevent their progress. This example demonstrates snapshot
validation rather than a general progress guarantee.

**The new idea:** correctness can come from rejecting an inconsistent
observation before using it. Preventing every intermediate observation is
not always necessary.

**Try:** walk through a reader that starts before the writer, reads one old
field and one new field, then compares its two versions. Identify why it
must reject that attempted snapshot.

### Prove the snapshot under the actual operations used

Use the sequentially consistent ordering of this teaching program to reason
about one total order of its atomic operations. A serialized writer makes
the version odd before changing fields and even after finishing. A reader
accepts only an even starting version that equals its ending version.

If a writer's update falls between the two version checks, the ending version
must differ or indicate an update in progress, assuming it cannot wrap around
and return to the starting value during that interval. If the writer finishes
before the reader's first check, the reader can take the completed new fields.
If it begins after the second check, the local copies already belong to the
completed earlier version. The reader uses those local copies rather than
rereading shared fields after validation.

The writer mutex is important even though readers do not take it. With two
uncoordinated writers, one writer could increment an odd version to even
while the other is still modifying data. “Even means stable” would then be
false. The marker's meaning is a protocol invariant that writer serialization
helps maintain.

### Two distinct rejected observations

First, the reader may see an odd starting version. It need not read the
payload at all; it retries because the protocol announces an active update.
Second, it may begin at an even version, collect fields, and later discover
a changed version. It rejects that attempted snapshot, including any mixed
values it temporarily held.

The second case is the conceptual advance. The reader need not prevent the
writer from running while it collects candidate values. It must refuse to
use those values as a valid snapshot when the evidence fails.

That is safe in this listing because every payload access is itself atomic.
A reader cannot perform undefined ordinary C races and retroactively legalize
them by rejecting the result. Validation protects the higher-level meaning;
it is not a time machine that repairs invalid language operations.

### Read the retry loop's control flow carefully

The uploaded `atomic_state_race.c` uses `continue` inside a `do/while`. In C,
that jumps to the loop condition, not directly to the top of the body. Its
condition begins with `(v1 & 1) || ...`, so short-circuit evaluation avoids
using `v2` on the odd-version path before `v2` has been assigned. That subtle
fact matters when reviewing the original.

The teaching version uses `for (;;)` with explicit acceptance and retry
paths, making that reasoning easier. It also uses `nanosleep` rather than
relying on visibility of the older `usleep` declaration under a strict
feature-macro setting, and widens a counter that can grow rapidly in a busy
retry loop. These edits are documented; they are not a claim that the
source's specific `continue` path necessarily reads an uninitialized `v2`.

**Check:** after a reader has copied a valid old version, does a writer
starting immediately afterward invalidate those private copies? No. The
claim is that they form a consistent version, not that they remain the most
recent values forever.

## 17. When the validation protocol cannot be trusted

The version counter works because every writer follows its rules. A writer
that changes a balance without changing the version can fool readers that
trust the counter. This is a change in assumptions, not a failure of the
atomic load instruction.

A Byzantine participant can behave arbitrarily: lie, withhold a response,
or give contradictory answers to different parties. Malicious intent is
one possible cause; arbitrary malfunction is another.

The first replica exercise made three replicas return `(400, 600)` and a
faulty replica return `(300, 700)`. Both reports preserve the total of 1000,
but only the former executes the requested transfer of 100.

The coordinator chose the earliest recorded response. That rule can accept
the wrong transition even though all thread accesses are synchronized.

**Source program — `first-reply.c`:**

```c
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define REPLICAS 4

struct Account {
    int checking;
    int savings;
};

struct Reply {
    int replica_id;
    struct Account state;
    struct timespec arrived;
};

struct Reply replies[REPLICAS];

void delay_ms(long ms) {
    struct timespec t = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}

void *replica(void *arg) {
    int id = *(int *)arg;
    struct Account local = {500, 500};

    if (id == 3) {
        // Byzantine replica: deliberately reports a false transition
        local.checking = 300;
        local.savings = 700;
        delay_ms(1);
    } else {
        // Honest replicas execute the requested $100 transfer
        local.checking -= 100;
        local.savings += 100;
        delay_ms(50 + id * 10);
    }

    replies[id].replica_id = id;
    replies[id].state = local;
    clock_gettime(CLOCK_MONOTONIC, &replies[id].arrived);

    return NULL;
}

int arrived_before(struct timespec a, struct timespec b) {
    if (a.tv_sec != b.tv_sec) return a.tv_sec < b.tv_sec;
    return a.tv_nsec < b.tv_nsec;
}

int main(void) {
    pthread_t threads[REPLICAS];
    int ids[REPLICAS];

    for (int i = 0; i < REPLICAS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, replica, &ids[i]);
    }

    for (int i = 0; i < REPLICAS; i++)
        pthread_join(threads[i], NULL);

    // FLAW: trust whichever replica responded first
    int winner = 0;

    for (int i = 1; i < REPLICAS; i++) {
        if (arrived_before(replies[i].arrived, replies[winner].arrived))
            winner = i;
    }

    for (int i = 0; i < REPLICAS; i++) {
        printf("Replica %d: checking=%d savings=%d total=%d\n",
            i, replies[i].state.checking, replies[i].state.savings,
            replies[i].state.checking + replies[i].state.savings);
    }

    struct Account accepted = replies[winner].state;

    printf("\nAccepted replica: %d\n", winner);
    printf("Final checking:  %d\n", accepted.checking);
    printf("Final savings:   %d\n", accepted.savings);

    return 0;
}
```

Each worker writes a separate reply element. The coordinator joins all
workers before reading their replies. Adding a mutex to these accesses does
not repair an acceptance rule that trusts the wrong evidence.

The source's objection was productive: this exercise mostly demonstrated
bad result selection, rather than a new shared-state concurrency problem.
Comparing compatible replies or calculating the expected transition locally
would address that demonstration. It did not yet expose competing decisions.

The redesigned exercise therefore introduces **two individually valid
proposals for one log position**. This brings local synchronization back
into the problem while giving replica agreement a separate responsibility.

**Takeaway:** internal consistency, correct execution of a requested
operation, and agreement between participants are distinct properties.

### A total of 1000 does not identify the requested operation

The first replica example preserves an accounting invariant even in its
faulty response. Both 400/600 and 300/700 total 1000. Only one is the result
of moving exactly 100 from an initial 500/500 state. An invariant that rules
out some bad results need not characterize every valid transition completely.

This is a useful discipline in designing checks. A checksum, a conserved
quantity, a type check, or agreement between replicas can establish one
property without establishing every property the application cares about.
Ask what alternative histories still satisfy the check.

In this toy example, a coordinator that knows the starting balances and the
requested transfer could calculate the expected answer itself. That is why
the learner objected that merely comparing predetermined reports was not
yet the intended concurrency problem. The redesign introduced conflicting
proposals so the votes had something genuinely contested to decide.

### Trust cannot be repaired by scheduling alone

The version counter assumes writers announce changes correctly. A faulty
writer that changes the data without changing the version can defeat that
evidence. Adding more atomic operations to the reader does not force the
writer to obey the contract.

The Byzantine model permits arbitrary behavior by the faulty participant.
Within the simulator, however, that participant is represented by a deliberately
special-cased function in one address space. We are assuming the rest of the
process's memory and code are not arbitrarily overwritten by that function.
The exercise models one aspect of Byzantine messages; it does not isolate a
malicious C thread that has access to all process memory.

This boundary deserves to be explicit because otherwise “we tolerate a liar”
could sound like a memory-security claim the simulator does not supply.
Independent processes or machines, stable identities, and appropriate message
validation introduce additional boundaries in a real deployment.

### Agreement is also not a truth oracle

If three replicas correctly execute an agreed deterministic transition on
valid input, their compatible reports can be useful evidence under a fault
model. If all three share an application bug or are fed a false real-world
input, agreement alone does not prove the outside-world fact was true.

The course will use quorum intersection to establish the impossibility of
two conflicting accepted decisions under specified rules. That is a precise
agreement property. It should not be inflated into “the majority is always
right” or “authentication proves that a signed claim is truthful.” A signature
can identify an endorser while that endorser deliberately lies.

## 18. Lab: conflicting proposals and accidental double voting

One account starts with 500. Proposal A requests a payment of 300; B requests
400. Either can be accepted from the initial balance, but both cannot be
honored together. Neither proposal is inherently “the bad one.” They conflict
for the same decision slot.

There are four replicas. R0–R2 are intended to vote once for this position.
R3 is faulty and approves both. A proposal is treated as committed after
three distinct replica approvals.

**Intentionally flawed source program — `bft_flawed.c`:**

```c
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define REPLICAS 4
#define BYZANTINE 3
#define QUORUM 3
#define UNVOTED -1

struct Replica {
    atomic_int voted_for;
    pthread_barrier_t barrier;
};

struct Proposal {
    int id;
    int amount;
    int approvals;
    int votes[REPLICAS];
};

struct Replica replicas[REPLICAS];

int request_vote(int replica_id, int proposal) {
    if (replica_id == BYZANTINE)
        return 1; // Faulty replica approves both proposals

    struct Replica *r = &replicas[replica_id];

    int observed = atomic_load(&r->voted_for);

    // Force both proposers to reach this point
    pthread_barrier_wait(&r->barrier);

    if (observed == UNVOTED) {
        atomic_store(&r->voted_for, proposal);
        return 1;
    }

    return observed == proposal;
}

void *propose(void *arg) {
    struct Proposal *p = arg;

    for (int i = 0; i < REPLICAS; i++) {
        p->votes[i] = request_vote(i, p->id);

        if (p->votes[i])
            p->approvals++;
    }

    return NULL;
}

int main(void) {
    pthread_t threads[2];

    struct Proposal a = { .id = 0, .amount = 300 };
    struct Proposal b = { .id = 1, .amount = 400 };

    for (int i = 0; i < REPLICAS; i++) {
        atomic_init(&replicas[i].voted_for, UNVOTED);

        if (i != BYZANTINE)
            pthread_barrier_init(&replicas[i].barrier, NULL, 2);
    }

    pthread_create(&threads[0], NULL, propose, &a);
    pthread_create(&threads[1], NULL, propose, &b);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    printf("\n--- REPLICA VOTES ---\n");

    for (int i = 0; i < REPLICAS; i++) {
        printf("R%d: A=%s B=%s\n", i,
            a.votes[i] ? "YES" : "NO",
            b.votes[i] ? "YES" : "NO");
    }

    int commit_a = a.approvals >= QUORUM;
    int commit_b = b.approvals >= QUORUM;

    printf("\n--- CONSENSUS RESULTS ---\n");
    printf("A: %d votes -> %s\n", a.approvals, commit_a ? "COMMIT" : "REJECT");
    printf("B: %d votes -> %s\n", b.approvals, commit_b ? "COMMIT" : "REJECT");

    if (commit_a && commit_b) {
        printf("\nSAFETY FAILURE: conflicting transactions finalized!\n");
        printf("Balance if both honored: $%d\n", 500 - a.amount - b.amount);
    }

    for (int i = 0; i < REPLICAS; i++) {
        if (i != BYZANTINE)
            pthread_barrier_destroy(&replicas[i].barrier);
    }

    return 0;
}
```

### The atomic object does not make the vote operation atomic

At each honest replica, both proposer threads load `UNVOTED`. The barrier
forces them both to reach that point before either continues. Each then
records its proposal and returns approval.

| Step | Proposer A | Proposer B |
| --- | --- | --- |
| Observe | Reads `UNVOTED` | Reads `UNVOTED` |
| Rendezvous | Waits at replica barrier | Waits at replica barrier |
| Decide | Stores A, returns YES | Stores B, returns YES |

Only one value remains in `voted_for`, but both callers have received a YES.
The final stored value does not erase an approval already returned.

The new validation run reproduced four approvals for A and four for B.
The program printed its safety failure and a hypothetical downstream balance
of -200. It models vote decisions; it does not actually perform bank payments.

The requirement is **one honest replica must not approve conflicting
proposals for the same slot**. Synchronization has to protect that entire
decision, not just each load or store individually.

### Returning YES creates information outside `voted_for`

The flawed implementation is especially instructive because inspecting the
last stored vote can conceal the failure. A and B both observe `UNVOTED`,
then each stores its ID and returns YES. The object eventually contains one
ID, but there are now two positive responses in the proposal records.

The property we need is about the accepted history: one honest replica may
not authorize both conflicting proposals for this slot. It is not merely
“the integer stores one integer at a time.” Atomic storage gives the latter
trivial property. The check-and-record operation must give the former.

The replica barrier deliberately makes this difference visible. Both handlers
must finish the atomic load before either can pass the barrier. That guarantees
the shared premise in the flawed first round without introducing a C data
race on the atomic vote. It is a controlled algorithmic failure.

### A quorum needs distinct participants

Suppose a proposer retries the same request three times and receives YES
from R0 each time. Those are not three independent replica approvals. A
certificate or quorum tally must account for identities and the relevant
instance/slot, round, and proposal. Repeated transport messages do not create
new voters.

Our array representation makes that easy to see: each replica has one slot
in a proposal's `votes` array, and approvals are collected from distinct
indices. Networking will make this bookkeeping more explicit because duplicate
responses and retries can occur.

### Why two quorums overlap in an honest replica

Each quorum contains three of the four replicas. Even if the first contains
R0/R1/R3 and the second R1/R2/R3, they overlap at R1 and R3. With at most
one arbitrary faulty participant, at least one of those two is honest.

If the honest member refuses conflicting approvals under the protocol's
rules, the two conflicting quorums cannot both be valid. Notice how the
mathematics depends on a local behavior rule. If a bug lets honest R1
approve both, counting to three does not rescue the design. This is exactly
what the flawed program demonstrates.

Nor is the number three a universal rule for every distributed experiment.
Three-of-four here is the one-fault Byzantine-inspired setting. Later we use
two-of-three as an election majority in a crash-fault learning model. The
numbers come from different assumptions and should not be interchanged
because both examples use the word “quorum.”

**Exercise:** if one faulty replica signs both A and B, does it alone create
two three-vote certificates? No. It still needs two additional distinct
endorsements for each. The intersection argument explains why the honest
rules matter.

## 19. Repairing the right critical section

The source explored three useful stages before reaching the correct repair.

### Stage A: a global mutex is a valid repair

The learner removed the forcing barrier and put a common mutex around each
call to `request_vote`. This serializes the entire check-and-record operation,
so it prevents accidental honest double voting in this simulation.

Its cost is scope: requests to R0 and R2 wait on the same lock even though
they concern independent replica state. This is the ticket-table comparison
appearing again.

### Stage B: a per-replica lock with an early unlock is still broken

Moving to one mutex per replica permits independent replicas to work in
parallel. But this attempted placement leaves the decision outside the lock:

```c
pthread_mutex_lock(&r->lock);
int observed = atomic_load(&r->voted_for);
pthread_mutex_unlock(&r->lock);

if (observed == UNVOTED) {
    atomic_store(&r->voted_for, proposal);
    return 1;
}
```

A can read `UNVOTED` and unlock. B can then read `UNVOTED` and unlock. Both
can subsequently approve. A correct-looking run merely missed that schedule.

### Stage C: extend the lock, then release it on every path

The repaired critical section covers observation, decision, and state change.
It computes the response, unlocks, then returns. Moving the unlock below an
early `return` would instead leave the mutex locked forever.

The other deadlock encountered in this progression was the two-party barrier
inside an exclusive lock. The owner waits at the barrier for a participant
that cannot enter because the owner holds the mutex.

Also preserve the API correction:

```c
pthread_mutex_init(&replicas[i].lock, NULL);
```

The second argument is an attributes pointer or `NULL`.
`PTHREAD_PROCESS_PRIVATE` is an attribute value, not that pointer.

### Complete consolidated repair

This teaching version combines the already-discussed faulty behaviors into
three modes. It removes unused barriers, uses ordinary `int` replica state
under the mutex, and destroys the mutexes after both threads finish. Each
proposal structure is owned by its proposer thread; main reads it after join.

It does **not** add recovery rounds or solve the next split-vote challenge.

**Edited consolidation — `bft_fixed.c`:**

```c
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define REPLICAS 4
#define BYZANTINE 3
#define QUORUM 3
#define UNVOTED -1

enum { DOUBLE_VOTE, WITHHOLD, OPPOSITE };
int fault_mode = DOUBLE_VOTE;

struct Replica {
    int voted_for;
    pthread_mutex_t lock;
};

struct Proposal {
    int id;
    int amount;
    int approvals;
    int votes[REPLICAS];
};

struct Replica replicas[REPLICAS];

int request_vote(int replica_id, int proposal) {
    if (replica_id == BYZANTINE)
        return fault_mode == DOUBLE_VOTE;
    struct Replica *r = &replicas[replica_id];
    pthread_mutex_lock(&r->lock);
    int observed = r->voted_for;
    int approved = (observed == UNVOTED || observed == proposal);
    if (observed == UNVOTED)
        r->voted_for = proposal;
    pthread_mutex_unlock(&r->lock);
    return approved;
}

void *propose(void *arg) {
    struct Proposal *p = arg;
    for (int i = 0; i < REPLICAS; i++) {
        if (i == BYZANTINE && fault_mode == OPPOSITE)
            p->votes[i] = !p->votes[i - 1];
        else
            p->votes[i] = request_vote(i, p->id);
        p->approvals += p->votes[i];
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "double") && strcmp(argv[1], "withhold") && strcmp(argv[1], "opposite"))) {
        fprintf(stderr, "usage: %s [double|withhold|opposite]\n", argv[0]);
        return 2;
    }
    if (argc == 2)
        fault_mode = !strcmp(argv[1], "withhold") ? WITHHOLD : !strcmp(argv[1], "opposite") ? OPPOSITE : DOUBLE_VOTE;
    pthread_t threads[2];
    struct Proposal a = { .id = 0, .amount = 300 };
    struct Proposal b = { .id = 1, .amount = 400 };
    for (int i = 0; i < REPLICAS; i++) {
        replicas[i].voted_for = UNVOTED;
        pthread_mutex_init(&replicas[i].lock, NULL);
    }
    pthread_create(&threads[0], NULL, propose, &a);
    pthread_create(&threads[1], NULL, propose, &b);
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    for (int i = 0; i < REPLICAS; i++)
        printf("R%d: A=%s B=%s\n", i, a.votes[i] ? "YES" : "NO", b.votes[i] ? "YES" : "NO");
    int commit_a = a.approvals >= QUORUM;
    int commit_b = b.approvals >= QUORUM;
    printf("A: %d votes -> %s\n", a.approvals, commit_a ? "COMMIT" : "REJECT");
    printf("B: %d votes -> %s\n", b.approvals, commit_b ? "COMMIT" : "REJECT");
    for (int i = 0; i < REPLICAS; i++)
        pthread_mutex_destroy(&replicas[i].lock);
    return commit_a && commit_b;
}
```

```bash
./bft_fixed double
./bft_fixed withhold
./bft_fixed opposite
```

Using an ordinary `int` is safe here because all concurrent honest-replica
accesses use that replica's mutex. Thread creation publishes the initial
state before the workers run; joining them precedes cleanup.

### Why three approvals protect this one decision slot

Two sets of three distinct replicas chosen from four overlap in at least:

```text
3 + 3 - 4 = 2 replicas
```

At most one is Byzantine, so at least one overlapping member must be honest.
If both conflicting proposals committed, that honest member would have had
to approve both. The repaired local rule prevents that.

This is a safety argument under the toy model's assumptions: distinct stable
replica identities, no forged votes, at most one faulty participant, and
honest votes remembered for this slot. Repeated tests complement that
argument; they do not replace it.

In a real distributed deployment, local array indices and function returns
would not supply authentication, independent failure isolation, persistent
votes after crashes, or safe protocol changes between rounds. The simulator
is BFT-inspired; it is not a complete BFT protocol.

### The global mutex solution was legitimate

The learner initially described the outer mutex solution as perhaps “not
allowed.” But the relevant question is what it establishes. With the forcing
barrier removed and both proposer threads using the same mutex around the
whole request, the honest replica's decision cannot be interrupted in the
forbidden check/store interval. That is a valid local repair in this model.

Moving the lock into each replica improves the scope of exclusion. It lets
an A→R0 request overlap a B→R2 request while still serializing two requests
for R0. The refinement preserves the proof for conflicting accesses and
reduces coordination between unrelated state.

This is the same architectural lesson as the ticket table. “One global lock
works” and “one global lock unnecessarily serializes independent operations”
can both be true. Correctness is not invalidated merely because a more
scalable design exists.

### Two repairs that go wrong in different ways

The early-unlock attempt has a safety problem. It leaves the conditional
state change outside protection. A thread can hold a locally cached
`UNVOTED` observation after another thread is allowed to make the same
observation. Both later approve.

The early-return attempt has a progress problem. Extending the critical
section while leaving `return 1` before the unlock can make the owner leave
without releasing the mutex. The other thread then waits indefinitely.
These are not the same defect and should not receive the same explanation.

One way to make the repaired function easier to audit is to have one exit
point after the unlock. Compute `approved`, optionally change the vote,
unlock, and return that computed result. This makes every branch's ownership
lifetime visible in the function's compact shape.

### Protect the predicate's scope, not the print statement

A `printf` placed under the lock can produce reassuringly ordered logs while
the actual decision remains outside it. Conversely, moving or removing the
print may expose a bad schedule that was already allowed. Logging is evidence
about events reached by that execution, not the ownership rule itself.

The uploaded `bft_fixed.c` includes the now-correct full decision scope, but
still carries unused barriers and the `PTHREAD_PROCESS_PRIVATE` argument at
initialization. On implementations where that macro is zero, the latter may
happen to behave like a null pointer constant. It is still the wrong API
expression to teach; use `NULL` for default mutex attributes. The intent and
portability of the call should not depend on an incidental macro value.

**Review exercise:** draw a line at every return in a lock-owning function.
For each, ask whether this path still owns the lock. Then inspect every
piece of state used in the decision and ask whether it can change after
its protection ends. These two passes find different classes of defect.

## 20. Safety without progress

A system can avoid contradictory commits while being unable to commit
anything.

| Property | Question | Current toy protocol |
| --- | --- | --- |
| Safety | Can both conflicting proposals commit? | Prevented under the stated one-slot assumptions |
| Liveness | Will an eligible proposal eventually commit? | Not guaranteed |

If R3 withholds its vote and the honest replicas split A/B/A, A has two
approvals and B has one. Repeating the same requests does not help because
the honest replicas remember their votes.

Withholding alone does not force a stall. If all three honest replicas
support A, A still reaches the three-vote threshold without R3.

### The learner's opposite-vote experiment

The latest code makes R3 return the opposite of R2 for each proposal:

```c
p->votes[i] = !p->votes[i - 1];
```

For each proposal, R2 and R3 contribute exactly one approval between them.
The remaining distinction is what happened at R0 and R1.

| Replica | A in a stalled example | B in that example |
| --- | --- | --- |
| R0 | YES | NO |
| R1 | NO | YES |
| R2 | NO | YES |
| R3, opposite R2 | YES | NO |
| **Total** | **2** | **2** |

If R0 and R1 instead support the same proposal, that proposal gets their
two approvals plus the one supplied by R2/R3: three approvals. Thus the
opposite-vote rule permits a 2–2 split; it does not guarantee either a stall
or a winner for every schedule.

Both proposer loops visit replicas in the same order. In ordinary runs,
one thread may finish several requests before the other advances. Per-replica
locking permits different winners at different replicas; it does not force
that interleaving to occur in the runs observed.

### Why retrying or resetting is not a recovery protocol

Retrying unchanged requests obtains unchanged remembered votes. Resetting
everyone to `UNVOTED` creates a different risk: an earlier vote may already
have contributed to a valid decision, while another participant or delayed
message still carries evidence of it.

Safe progress across attempts requires rules for retaining commitments and
interpreting old evidence. Rounds/views, leaders, and quorum certificates
were introduced as future concepts. Their implementation is beyond the
completed progression in this source snapshot.

### Halting can preserve safety vacuously

A system that approves nothing cannot approve two conflicting decisions.
That is why the word “safe” alone is not enough to describe a useful service.
We also need a progress claim with assumptions about available participants,
message delivery, and scheduling.

The first attempted guaranteed stall rejected every odd-numbered replica
and skipped the faulty one. That left only two participants able to approve,
while the threshold was three. The outcome was explained by arithmetic,
not by a subtle collective split among three functioning honest replicas.
The experiment changed the population of participants on which the intended
fault assumption relied.

The target experiment is more informative: all three honest replicas follow
their vote-once rule, but their valid decisions divide between A and B.
The faulty fourth withholds approval. A gets two and B one, or the reverse.
The local proof works, yet the protocol has no rule ensuring a common winner.

### Opposing one replica is not the same as opposing the whole quorum

The opposite-to-R2 behavior contributes one YES between R2 and R3 for each
proposal. Whether a proposal reaches three then depends on the other two
honest replicas. If R0 and R1 agree, their two approvals plus that one make
three. If they disagree, each proposal gets one from R0/R1 and one from
R2/R3, making a two–two split.

The learner's initial runs often showed one proposer sweeping the honest
replicas. That was a permitted schedule, not a rule imposed by the per-replica
mutexes. Making different outcomes reliable requires controlling the schedule
at appropriate boundaries, not concluding the unobserved outcome is impossible.

### Progress failure needs a stated time model

The one-shot toy permanently records its votes. An ordinary retry of the
same requests has no reason to change them, so a failed round can remain
stalled forever. More generally, a slow response is not automatically a
failed participant. A timeout says the observer has not received the event
within a chosen interval; it is not direct evidence of what every other
participant did.

The next chapters make both points concrete: first by constructing a split
without undefined behavior, then by distinguishing another local attempt
from recovery when different observers have incomplete information. The
old outline stopped before those experiments. The expanded source now gives
us the next part of the course.

## 21. A reliable split without undefined behavior

The next task was to produce a split while keeping all three honest replicas
correct. Random sleeps made it more likely but rarely made it dependable.
A useful test should arrange the relevant interleaving without introducing
an unrelated C data race.

One attempted scheduling mechanism used a global `ready_to_proceed` flag and
a condition variable while waiting under different per-replica mutexes. It
exposed the desired voting pattern, but did not give that pattern a reliable
synchronization explanation. The flag's accesses did not all use the same
protection. Concurrent waits on one condition variable also cannot simply
use unrelated associated mutexes.

The distinction is important: intentionally making request order
nondeterministic is legitimate. Intentionally allowing undefined behavior
makes it difficult to know which phenomenon the experiment measures.

### Synchronize stages, not the contested decision

Use different request orders for the two proposers, with one shared stage
barrier after each request:

| Stage | A requests | B requests | What the gate establishes |
| --- | --- | --- | --- |
| 1 | R0 | R1 | Both initial claims finish before stage 2 |
| 2 | R1 | R0 | Both encounter the other's recorded first claim |
| 3 | R2 | R2 | The remaining honest vote is a real concurrent contest |

The stage-one requests target different honest replicas, so both can succeed.
The barrier then prevents either proposer from running ahead to collect the
other's initial replica before that first claim exists. R2 remains contested;
either proposer can win it. With R3 withholding approval, the result is two
votes versus one, never the required three.

**Additional controlled teaching experiment — `split_vote.c`:** this is an
editorial consolidation of the scheduling idea developed in the thread. It
is not presented as a byte-for-byte copy of the learner's final source.

```c
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#define REPLICAS 4
#define QUORUM 3
#define UNVOTED -1

struct Replica {
    int voted_for;
    pthread_mutex_t lock;
} replicas[REPLICAS];

struct Proposal {
    int id;
    int votes[REPLICAS];
    int approvals;
};

pthread_barrier_t step_gate;

int request_vote(int replica, int proposal) {
    if (replica == 3) return 0; // One faulty participant withholds its vote
    struct Replica *r = &replicas[replica];
    pthread_mutex_lock(&r->lock);
    int approved = (r->voted_for == UNVOTED || r->voted_for == proposal);
    if (r->voted_for == UNVOTED) r->voted_for = proposal;
    pthread_mutex_unlock(&r->lock);
    return approved;
}

void *propose(void *arg) {
    struct Proposal *p = arg;
    const int orders[2][3] = {{0, 1, 2}, {1, 0, 2}};
    for (int step = 0; step < 3; step++) {
        int replica = orders[p->id][step];
        p->votes[replica] = request_vote(replica, p->id);
        p->approvals += p->votes[replica];
        pthread_barrier_wait(&step_gate); // Same stage gate; no replica lock held
    }
    return NULL;
}

int main(void) {
    struct Proposal a = {.id = 0}, b = {.id = 1};
    pthread_t ta, tb;
    pthread_barrier_init(&step_gate, NULL, 2);
    for (int i = 0; i < REPLICAS; i++) {
        replicas[i].voted_for = UNVOTED;
        pthread_mutex_init(&replicas[i].lock, NULL);
    }
    pthread_create(&ta, NULL, propose, &a);
    pthread_create(&tb, NULL, propose, &b);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    for (int i = 0; i < REPLICAS; i++)
        printf("R%d: A=%d B=%d\n", i, a.votes[i], b.votes[i]);
    printf("A=%d B=%d quorum=%d\n", a.approvals, b.approvals, QUORUM);
    assert(a.approvals < QUORUM && b.approvals < QUORUM);
    assert(a.approvals + b.approvals == 3);
    assert(a.votes[0] == 1 && b.votes[1] == 1);
    for (int i = 0; i < 3; i++) assert(a.votes[i] + b.votes[i] == 1);
    for (int i = 0; i < REPLICAS; i++) pthread_mutex_destroy(&replicas[i].lock);
    pthread_barrier_destroy(&step_gate);
    return 0;
}
```

### Why one stage barrier matters

Do not replace the shared gate with “whichever replica's barrier this worker
just visited” after giving workers different orders. A could wait at R0's
barrier while B waits at R1's. Each barrier expects two participants, and
neither can reach the other's barrier. That would be a deadlock of the test
harness rather than the intended lack of a commit quorum.

Similarly, do not hold a replica's exclusive mutex while waiting for the
other proposer to enter that same replica. The first participant would be
waiting for progress it has itself excluded.

The actual source also encountered `if (n > i > 0)`. C parses this as
`(n > i) > 0`; it is not mathematical chained-comparison notation. If a range
is intended, write both comparisons with `&&`. A misleading condition can
accidentally put a barrier on every iteration, changing the test's schedule.

### A condition variable waits for a predicate

A conventional pattern looks like this:

```c
pthread_mutex_lock(&gate_lock);
while (!may_proceed)
    pthread_cond_wait(&changed, &gate_lock);
/* The predicate is true while the required lock is held. */
pthread_mutex_unlock(&gate_lock);
```

The actor changing `may_proceed` participates in that same mutex protocol.
The wait releases the mutex while blocked and reacquires it before returning.
The loop matters because a wakeup does not by itself prove the predicate is
true. A condition variable is a way to wait for protected state to change;
it is not an extra Boolean-valued atomic instruction.

**Check your understanding:** could you get the same split by making two
replicas permanently refuse? Yes, but it would establish a different result:
there are fewer potential approvals than the threshold. The controlled
experiment establishes failure of progress despite all three honest replicas
making valid, usable decisions.

## 22. Another round is not a vote on who was right

After producing a failed round, the learner added retries and reset voting
state. This introduced a surprisingly difficult language problem: what does
it mean to make the nodes “agree” next time?

We need to separate three decisions:

- Which proposal is valid for the application?
- Which eligible proposal will occupy this particular decision slot?
- Which participants, if any, have violated the protocol?

A disagreement between honest replicas does not answer the third question.
R0 may receive A first while R1 receives B first. Both can follow the rules.
Nor does a two–one distribution establish that the two-vote proposal is the
inherently correct one. It is information about this attempt's ordering.

### Work through the learner's two possible outcomes

Suppose a failed round has A/A/B among the honest replicas and an abstention
from the faulty one. No proposal has three approvals. In a simplified,
fully observed test with no earlier commitment or other protocol restriction,
a later attempt could validly choose A or B.

| Replica | Failed round | Later outcome 1 | Later outcome 2 |
| --- | --- | --- | --- |
| R0 | A | A | B |
| R1 | A | A | B |
| R2 | B | A | B |
| R3 | Abstains | Abstains | Abstains |
| Accepted result | None | A | B |

The aim is not to punish R2 for its earlier minority vote. The aim is to
establish one safe decision. We might reduce contention by arranging one
proposer for a later attempt, but the rule for choosing a *safe* proposal
has to be defined when previous evidence can constrain the choice.

### Why the reset experiments were still useful

The learner's first retry loop repeatedly observed identical votes because
replica state had not been reinitialized in the relevant scope. Resetting
only one variable does not necessarily reset proposal totals, vote arrays,
or per-replica remembered decisions. The round's state needs a clear owner
and initialization boundary.

After the workers finish and the centralized coordinator observes a failed
attempt, it can reset the toy and run another attempt. Removing a forcing
barrier in the later round lets one proposer get ahead; that can make a
quorum common. It is an observation about a favorable schedule, not a proof
that unlimited repetition guarantees progress.

Every worker also has to reach each required phase gate. If only the failed
proposer enters a two-party barrier while the successful one exits, the
barrier waits forever. Logging “waiting for barrier” can therefore describe
a real control-flow mismatch rather than an atomic state that “needs more
resetting.” The required participant set is part of the protocol.

### The question that corrected the assignment

**Reader, condensed:** If three already agreed and the program knows that,
why keep voting in another round?

**Explanation:** In this centralized simulation, there is no reason. Main
joins the handlers, inspects the complete result, and can stop when it has
the required quorum. The source eventually corrected the exercise: it had
been asking for a danger that the current model did not actually expose.

Different program executions choosing different winners are not contradictory
commits within one instance. Each invocation starts from its own initialized
state. A/A/A in one run and B/B/B in a different run do not violate the
one-decision property for either run.

This correction is part of the lesson, not incidental conversational noise.
A more sophisticated mechanism must answer a problem present in the model.
Before adding history-preservation machinery, introduce the incomplete
knowledge that makes that history difficult to observe.

## 23. Quorum evidence and incomplete knowledge

Consider the same actual votes: R0, R1, and R2 endorsed A; R3 abstained.
Now let two independent coordinators receive different subsets of the
responses.

| Evidence from round 0 | Coordinator X | Coordinator Y |
| --- | --- | --- |
| R0's A approval | Received | Received |
| R1's A approval | Received | Received |
| R2's A approval | Received | Delayed or missing |
| What can be concluded locally? | Three approvals satisfy this toy's commit rule | Only two approvals have been observed |

Y has not proved that no quorum exists. It lacks a piece of evidence that
X already possesses. A timeout changes Y's knowledge about elapsed waiting,
not the actual fact of whether R2 voted or X collected the vote.

This is the missing ingredient from the centralized join-and-count simulation.
There, main can wait for all completed thread handlers and inspect their
records. Here, no observer automatically receives a complete snapshot of
another process's messages and decisions.

### What could go wrong with a naive new round?

Suppose X has enough evidence to finalize A for slot 42, but becomes
unavailable before Y learns that fact. Y initiates a new round. If the
protocol simply erases every vote and permits R0/R1/R2 to approve B, Y can
collect a new three-vote set for B in the same slot.

Each round separately obeyed “one vote per replica in that round.” Each
quorum contained three distinct participants. The overlapping honest
replicas changed their votes between rounds. Thus the one-round intersection
argument alone did not establish cross-round safety.

The new requirement is a rule constraining what those honest participants
may endorse after prior voting evidence. Production protocols use more
structure, such as phases, retained locks, and information exchanged during
changes of leadership. The exact safe rule depends on the protocol. Adding
an integer named `round` does not supply it automatically.

### History is a constraint, not a popularity ranking

The dialogue repeatedly returned to “subject to preserving earlier
commitments.” The careful interpretation is:

| Historical fact | Implication for this same decision slot |
| --- | --- |
| A received more votes in an uncommitted attempt | Does not alone make A the required winner |
| A has been finalized under the protocol | A conflicting result cannot later be finalized |
| This node has not seen a finalization | Does not prove no other node has sufficient evidence |
| Earlier protocol evidence imposes a safety restriction | Follow the restriction even if no final decision is locally known |

A real protocol may need to preserve evidence before finalization precisely
because another participant could know more. “No commitment in my local
records” is weaker than “a protocol proof establishes that this new proposal
cannot conflict with anything already decided.”

### Keep instance, round, and term distinct

A **slot or instance** identifies the decision being made. A **round/view**
identifies an attempt or coordinator epoch within the protocol. A **term**
in the later leadership exercise identifies a leadership epoch. Moving to a
new attempt is not permission to contradict an earlier binding decision for
the same slot. Nor is choosing a different value for a new independent slot
necessarily a contradiction.

**Reasoning exercise:** X has A's certificate; Y has only two A votes. What
must Y not infer from its timeout? That A is known uncommitted. What additional
problem must recovery solve? Gathering and preserving sufficient evidence so
that incomplete knowledge cannot authorize an incompatible decision.

The source developed this model conceptually, then moved to independent
machines. It did not complete a Byzantine view-change protocol. That boundary
remains explicit in this edition.

## 24. A supervisor chooses a leader; an old node does not learn it

The course then moved from shared globals to three Linux C nodes contacted
by a Python supervisor on Windows. This was a useful architectural change:
each node had its own memory and received only the messages actually delivered
to it. A global mutex could no longer make their observations identical.

The supervisor monitored response times and chose a preferred node, with a
rolling measurement window and a threshold intended to avoid excessive
switching. This is a useful selection policy for a controlled experiment.
Fastest recent response is not itself evidence of exclusive authority to
make future system decisions.

The small command vocabulary separated observations from actions: ping the
node, inspect its status, announce a term and leader, and send a job tagged
with a term. Nodes checked jobs against their local remembered leadership.

### The local rule can be correct and still be insufficient

The relevant acceptance condition resembled:

```c
if (request_term == node->term && node->leader_id == node->id) {
    /* Accept local work. */
}
```

Imagine R2 learned that it led term 1. Later the supervisor replaces it with
R1 for term 2, but the announcement to R2 is withheld.

| Node | Local term | Local leader | Response to a matching local-term job |
| --- | --- | --- | --- |
| R1 | 2 | R1 | Accepts |
| R2 | 1 | R2 | Also accepts |

The source reported this stale-leader behavior on the learner's LAN after
selective announcement loss and network delay. It is not a C memory race.
Each node can evaluate its local predicate accurately while the two predicates
express incompatible beliefs about current authority.

The reported job only changed local node state. Accepting two local jobs is
not yet proof of two contradictory entries committed to a replicated log.
It demonstrates the authority gap that a later commitment mechanism must
prevent from authorizing conflicting effects.

### Verify that the fault injector caused the intended fault

One experimental result was obscured by the supervisor's own `announce`
function. A loop printed that an announcement was dropped, but a separate
loop sent the announcement to all nodes anyway. The log described intended
fault injection rather than what the network-send path actually did.

Combining the skip decision with the send loop fixed that discrepancy. This
is the distributed version of the earlier logging lesson: a printed
interpretation is not sufficient evidence that the intended event occurred.
Check the path performing the action.

### What latency and silence can tell us

A larger response time may reflect load, scheduling, buffering, or a network
path. A missing response may reflect an unreachable node, a lost request, a
lost reply, or an action that happened without its reply being observed.
The observer cannot generally infer the remote operation's exact history
from silence alone.

A timeout is still useful: it lets a system stop waiting indefinitely and
attempt another course of action. The new course of action must be safe
under the uncertainty that remains. “We stopped waiting for the old leader”
and “the old leader is incapable of acting” are not equivalent statements.

**Check:** can another supervisor message force an isolated node to change
its belief? Not while that message cannot reach it. A robust design must
prevent unsafe accepted effects even when some beliefs remain stale.

## 25. Election authority and permission to commit work

The next proposed step removed the Windows machine's ability to unilaterally
appoint leaders. It could still measure, suggest candidates, and inject
failures; the Linux nodes would need to authorize leadership through peer
votes.

For a three-node crash-fault learning model, a candidate could seek two
votes, including its own. An honest node would cast at most one vote in a
term. The local state might include `current_term`, `voted_for`, and
`leader_id`. Those fields need transition rules, not merely declarations.

### Why one vote per term is useful

Two groups of two drawn from three nodes overlap in at least one node. If
all participants follow the rule, that overlapping node cannot authorize
two candidates in the same term. This is an election-safety argument for
that model. It assumes stable identities and that the remembered vote is not
forgotten in a way the model allows candidates to exploit.

If crash recovery is included, persistence becomes part of the problem.
A process that restarts and forgets its vote may vote again in the same term.
An in-memory integer is enough only for a model whose failure assumptions
exclude that history or otherwise handle it. The source had proposed the
peer-election rules, not finished this recovery implementation.

Three nodes and a two-node majority here do not supply the earlier
three-of-four Byzantine guarantee. The crash-fault experiment and the
Byzantine-inspired simulator must keep their failure assumptions separate.

### A newly elected leader need not instantly correct every node

R0 and R1 may communicate and authorize leadership in a newer term while R2
remains isolated with its old view. That temporary disagreement is not itself
proof that the election mechanism failed. The central requirement is what
work can be accepted as authoritative despite the stale view.

| Question | Mechanism whose contract must answer it |
| --- | --- |
| Who may claim leadership in this term? | Election votes and term-transition rules |
| Can a stale leader still commit new work? | Commitment acknowledgements, retained history, and resource-enforcement rules |
| Can a node act forever without renewal? | Any chosen lease/expiry protocol and its timing assumptions |
| Can a resource reject a stale actor? | A fencing rule enforced at that resource |

These rows are design questions, not a completed implementation. A local
comparison between a job's term and a node's own term cannot tell an isolated
node that a higher term now exists elsewhere.

A lease can limit authority in time under its actual clock and timing
assumptions. A fencing token can let a resource reject older authority after
it has accepted a newer token. Neither concept works merely because the
sender includes an integer: the actor authorizing the lease or enforcing the
fence must follow the necessary rules. Quorum authorization of work likewise
needs correct log/commit semantics, not just a count of arbitrary replies.

### The next protocol exercise remains open

The learner chose to implement transport and event handling before peer
voting. That is a sensible dependency: an election handler cannot be tested
cleanly while the server drops commands before reading them.

For a future implementation, predict the behavior of two simultaneous
candidates in the same term. Explain which local transition prevents double
voting, how newer terms affect existing state, and what survives a restart.
Then separately ask whether a previously authorized leader can still cause
external work. Election safety alone is not the complete state-machine
replication argument.

## 26. From an accepted socket to a complete command

The uploaded `serving.c` establishes a listening socket, accepts connections,
and closes them. It contains no application-level reply protocol. A successful
TCP connection to that program establishes that transport setup reached the
server; it does not promise a `PONG` or a vote response.

The latest shared exchange had moved further, attempting to multiplex clients
with `poll()`. Several mistakes made it a particularly useful exercise in
state ownership and partial progress.

### Connection establishment is not command arrival

The attempted accept branch registered a client for polling, then immediately
called the old request handler and closed the client. On a nonblocking socket,
there might not yet be request bytes. `recv` could return `EAGAIN` or
`EWOULDBLOCK`; the old line reader treated any non-positive return as a reason
to stop. The server could close before the client even sent its command.

| Event | What has been established | What has not |
| --- | --- | --- |
| `accept` succeeds | A client connection is available | A complete application command is present |
| Client is readable | A receive attempt can make progress or reveal stream state | A whole line necessarily fits in one read |
| `recv` returns bytes | Those bytes are available | They end at an application message boundary |
| Parser finds a newline | One complete line is available under this protocol | Every other connection is finished |
| `send` returns a positive count | That many bytes were accepted by the call | The entire response was sent or remotely processed |

Registering a descriptor transfers it into the event loop's lifecycle. Closing
it immediately while leaving its number in the poll array creates another
problem: the table contains a descriptor that is no longer the live connection
it claims to represent. Ignoring `POLLNVAL` can turn that into a busy or
capacity-exhausted loop.

The attempted client branch also read bytes without feeding them to the
command processor that generated replies. Two competing receive paths could
therefore consume a request while neither produced the expected response.
The fix begins with one receive path, one framing policy, and one place
where complete commands change node state.

### TCP preserves a byte stream, not your function calls

A client can send `PI` and later `NG\n`. It can also send `PING\nSTATUS\n`
in one call. The server must preserve an incomplete first fragment and parse
both complete lines in the second case. Neither behavior should require
changing the business logic for `PING`.

Similarly, if an output call sends only part of the queued reply, the
connection must retain the remainder and its offset. Retrying the entire
reply from byte zero can duplicate a prefix. Treating a temporary inability
to write as success can silently lose the suffix.

The useful per-client state is therefore more than an integer descriptor:
input bytes and length, output bytes and length, a sent offset, and lifecycle
information. The event loop owns this state across readiness notifications.

### Nonblocking does not mean background completion

`O_NONBLOCK` means the operation can return instead of waiting when it cannot
make progress. It does not arrange for C code to finish the operation later.
`poll` reports readiness; the program must attempt I/O, interpret the result,
and keep whatever state is needed for the next attempt.

For a stream receive of a nonzero requested length, distinguish:

| Result | Interpretation in this exercise |
| --- | --- |
| Positive count | Append the bytes and look for complete frames |
| Zero | Peer has ended its sending stream |
| `-1`, `EINTR` | Retry according to the operation's policy |
| `-1`, `EAGAIN`/`EWOULDBLOCK` | Keep connection state and wait for later readiness |
| Other error | Apply the connection's error/close policy |

Also, the third argument of `poll` is a timeout in milliseconds, not a port.
The parsed listening port must actually be used when constructing the bind
address. Both errors appeared in the latest debugging discussion and are
separate from the framing problem.

### A worked transport-only companion

The following optional teaching implementation demonstrates those mechanics
without implementing peer elections or consensus. It accepts newline-delimited
`PING` and `STATUS`, retains partial input/output, bounds its buffers, and
uses one thread to own its request counter. It binds to loopback for a
single-machine lab. Linux/WSL is the target; `MSG_NOSIGNAL` is used for sends.

This is newly written teaching code, not a claim that the source thread had
already completed its nonblocking server. The submitted `serving.c` and the
latest shared attempt remain archived separately.

**`poll_ping.c`:**

```c
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 16
#define INPUT_SIZE 1024
#define OUTPUT_SIZE 4096

struct Client {
    int fd;
    int peer_closed;
    char input[INPUT_SIZE];
    size_t input_len;
    char output[OUTPUT_SIZE];
    size_t output_len;
    size_t output_sent;
};

unsigned long requests = 0; // One event-loop thread owns this state

int make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return flags < 0 ? -1 : fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void close_client(struct Client *c) {
    if (c->fd >= 0) close(c->fd);
    *c = (struct Client){.fd = -1};
}

int queue_reply(struct Client *c, const char *line) {
    char reply[128];
    requests++;
    if (!strcmp(line, "PING")) snprintf(reply, sizeof(reply), "PONG\n");
    else if (!strcmp(line, "STATUS")) snprintf(reply, sizeof(reply), "requests=%lu\n", requests);
    else snprintf(reply, sizeof(reply), "ERR unknown command\n");
    size_t len = strlen(reply);
    if (len > sizeof(c->output) - c->output_len) return -1;
    memcpy(c->output + c->output_len, reply, len);
    c->output_len += len;
    return 0;
}

int read_client(struct Client *c) {
    for (;;) {
        size_t space = sizeof(c->input) - 1 - c->input_len;
        if (!space) return -1; // Bounded line: do not overrun or wait forever
        ssize_t n = recv(c->fd, c->input + c->input_len, space, 0);
        if (n > 0) {
            c->input_len += (size_t)n;
            c->input[c->input_len] = '\0';
            char *newline;
            while ((newline = memchr(c->input, '\n', c->input_len)) != NULL) {
                size_t used = (size_t)(newline - c->input) + 1;
                *newline = '\0';
                if (queue_reply(c, c->input) < 0) return -1;
                memmove(c->input, c->input + used, c->input_len - used);
                c->input_len -= used;
                c->input[c->input_len] = '\0';
            }
        } else if (n == 0) {
            c->peer_closed = 1;
            return 0; // Incomplete tail is discarded; complete replies still drain
        } else if (errno == EINTR) {
            continue;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // No bytes now; retain the partial line
        } else {
            return -1;
        }
    }
}

int write_client(struct Client *c) {
    while (c->output_sent < c->output_len) {
        ssize_t n = send(c->fd, c->output + c->output_sent, c->output_len - c->output_sent, MSG_NOSIGNAL);
        if (n > 0) c->output_sent += (size_t)n;
        else if (n < 0 && errno == EINTR) continue;
        else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
        else return -1;
    }
    c->output_len = c->output_sent = 0;
    return 0;
}

int main(int argc, char **argv) {
    char *end;
    long port = argc == 2 ? strtol(argv[1], &end, 10) : 5050;
    if (argc > 2 || (argc == 2 && (!argv[1][0] || *end)) || port < 1 || port > 65535) {
        fprintf(stderr, "usage: %s [port:1..65535]\n", argv[0]);
        return 2;
    }
    int listener = socket(AF_INET, SOCK_STREAM, 0), one = 1;
    if (listener < 0) { perror("socket"); return 1; }
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK), .sin_port = htons((unsigned short)port)};
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0 ||
        make_nonblocking(listener) < 0 || bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0 || listen(listener, MAX_CLIENTS) < 0) {
        perror("listener setup");
        close(listener);
        return 1;
    }
    struct Client clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = (struct Client){.fd = -1};
    printf("Listening on 127.0.0.1:%ld\n", port);
    fflush(stdout);
    for (;;) {
        struct pollfd fds[MAX_CLIENTS + 1] = {{.fd = listener, .events = POLLIN}};
        for (int i = 0; i < MAX_CLIENTS; i++) {
            fds[i + 1].fd = clients[i].fd;
            fds[i + 1].events = clients[i].peer_closed ? 0 : POLLIN;
            if (clients[i].output_len > clients[i].output_sent) fds[i + 1].events |= POLLOUT;
        }
        int ready = poll(fds, MAX_CLIENTS + 1, -1);
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        if (fds[0].revents & POLLIN) {
            for (;;) {
                int fd = accept(listener, NULL, NULL);
                if (fd < 0) {
                    if (errno == EINTR) continue;
                    if (errno != EAGAIN && errno != EWOULDBLOCK) perror("accept");
                    break;
                }
                int slot;
                for (slot = 0; slot < MAX_CLIENTS && clients[slot].fd >= 0; slot++) {}
                if (slot == MAX_CLIENTS || make_nonblocking(fd) < 0) close(fd);
                else clients[slot] = (struct Client){.fd = fd};
            }
        }
        for (int i = 0; i < MAX_CLIENTS; i++) {
            struct Client *c = &clients[i];
            short events = fds[i + 1].revents;
            if (c->fd < 0 || !events) continue;
            if (events & (POLLERR | POLLNVAL)) { close_client(c); continue; }
            if ((events & (POLLIN | POLLHUP)) && read_client(c) < 0) { close_client(c); continue; }
            if (c->output_len > c->output_sent && write_client(c) < 0) { close_client(c); continue; }
            if (c->peer_closed && c->output_len == c->output_sent) close_client(c);
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++) close_client(&clients[i]);
    close(listener);
    return 1;
}
```

Compile and run it in one terminal:

```bash
gcc -std=c11 -O2 -Wall -Wextra poll_ping.c -o poll_ping
./poll_ping 5050
```

Use Python in a second terminal; this client helper waits for a complete
reply line rather than assuming one `recv` contains it:

```python
import socket

with socket.create_connection(("127.0.0.1", 5050)) as s:
    s.sendall(b"PI")
    s.sendall(b"NG\nSTATUS\n")
    with s.makefile("rb") as replies:
        print(replies.readline().decode().strip())  # PONG
        print(replies.readline().decode().strip())  # requests=2
```

The request counter measures parsed commands in this local teaching server,
including an unknown command that receives an error. The implementation
closes connections on overlong lines or an exhausted output queue rather
than overrunning storage; it discards an incomplete tail after peer EOF.
These are explicit bounded teaching policies, not a general production
protocol. It does not provide authenticated requests, replicated durable
state, election terms, or authority to commit work.

### Why one thread can service multiple clients

While one client has only supplied `PI`, another can supply a complete `PING`.
The server need not block waiting for the first client's remaining bytes.
One event-loop thread can keep both connections in progress and process
complete commands one at a time. That eliminates simultaneous local writes
to its ordinary request counter without eliminating network concurrency.

The original three machines still run independently. Once they exchange
votes, this locally orderly implementation will face incomplete remote
knowledge again. Transport correctness is the prerequisite that lets us
study that next failure without confusing it with a dropped partial line.

## 27. Where this expanded edition stops

The coursework has moved beyond the previous outline's split-vote checkpoint.
The source now includes controlled split attempts, retry-round debugging,
the clarification about quorum evidence and incomplete knowledge, and a
reported stale-leader demonstration on independent LAN nodes.

The latest included exchange is the review of a C server combining `poll`
with an older request handler. Its immediate task is to keep accepted
connections alive, wait for client readiness, preserve framing state, and
return the expected reply. A peer-to-peer `REQUEST_VOTE` protocol was proposed
but not completed in the supplied source.

This book supplies additional controlled teaching examples and a worked
transport companion, clearly identified as editorial additions. They do not
retroactively change what the learner implemented in the conversation.

**Still open in the coursework:** peer-authorized elections, persistence and
recovery rules, a protocol preventing stale leaders from committing
conflicting work, and a complete Byzantine view-change/commit implementation.
ABA, hazard pointers, and epoch-based reclamation also remain future topics.

The checkpoint stores the exact final source message ID and an excerpt, not
an inferred message date. Future updates should append later completed
lessons while keeping corrections to existing chapters identifiable. The
new snapshot extends the continuation; it does not replace the original
foundation with a second recap.

## Appendix A. How the uploaded files map to the lessons

The sixteen submitted C files are retained byte-for-byte in `originals/`, with
a SHA-256 manifest in `sources/`. The `examples/` directory contains edited
teaching versions and supplemental programs; changes do not silently replace
the historical attempts. Every teaching listing appears in this Markdown.

| Submitted file | Read with | Important interpretation |
| --- | --- | --- |
| `big_atomic_energy.c` | Chapter 3 | Assembly probe; compile with `-c` or `-S`, not as an executable |
| `cas_claim.c` | Chapter 3 | One-shot claim plus the problematic exploratory jitter routine |
| `spin_lock.c` | Chapter 4 | Ordinary counter under an atomic-flag lock; teaching file is `spin_counter.c` |
| `cache_battle.c` | Chapter 5 | Exploratory logging/cross-reading; local thread-handle names hide counters |
| `cache_ping_pong.c` | Chapter 5 | Three-case layout benchmark; teaching file is `false_sharing.c` |
| `thread_relay.c` | Chapter 6 | Semaphore baton and independent finish barrier |
| `atomic_tickets.c` | Chapter 10 | Defined atomic operations but a broken combined availability decision |
| `reservations.c` | Chapter 13 | Stale seat snapshots plus ordinary conflicting writes; has C data races |
| `pessimistic_reservations.c` | Chapter 13 | Shared mutex protects the whole seat operation; initialized barrier is unused |
| `optimistic_reservations.c` | Chapter 13 | Strong CAS rejects one stale claim; failed acquisition is correct behavior |
| `tickets.c` | Chapter 14 | Global table mutex versus per-ticket ownership CAS; teaching file is `ticket_compare.c` |
| `atomic_state_race.c` | Chapter 16 | Atomic-payload versioned snapshot; teaching file is `snapshot.c` |
| `flawed_consensus.c` | Chapter 18 | Forced honest double voting from separate load/check/store |
| `global_mutex_consensus.c` | Chapter 19 | Valid broad exclusion after removal of the forcing barrier |
| `bft_fixed.c` | Chapter 19 | Full per-replica decision protected; initialization/cleanup edits explained |
| `serving.c` | Chapter 26 | Accept-and-close transport baseline, not a command/reply server |

The earlier generated counter programs provide the first lab even though
separate counter attachments were not included in this upload. The source
thread and prior manuscript supply that progression. The additional controlled
atomic-split counter, phase-gated split vote, and transport-only `poll_ping`
are newly written explanatory experiments, not historical learner submissions.

### Selected editorial changes

- Compact function braces and ordinarily readable calls, as requested.
- Explicit POSIX feature macros where barriers or clocks require them.
- `nanosleep` in the snapshot teaching version; no claim that a timed delay
  establishes thread ordering by itself.
- Correct mutex-attribute argument and cleanup in the voting teaching version.
- A simpler snapshot retry loop, without misdiagnosing the original
  short-circuit condition as necessarily reading an uninitialized `v2`.
- Layout reporting with `offsetof` rather than subtracting pointers to distinct
  subobjects in the cache display.
- Bounded transport state in the optional event-loop companion; it implements
  PING/STATUS only, not a consensus algorithm.

The submitted code is evidence about the learning progression, including its
mistakes. A title such as “fixed” is not accepted as a substitute for reviewing
the implementation and its assumptions.

## Appendix B. Guided review with explained answers

These exercises ask for a reason, not only a yes/no result. Pause before each
answer and identify the relevant state, observation, and acceptance boundary.

### 1. Two private locks around one counter

**Problem:** both workers call the same function. Inside it, each initializes
a mutex, locks it, and increments a shared ordinary counter. Why is this not
a repair?

**Reasoning:** the two invocations contain different mutex objects. The lock
operations do not contend, so the counter accesses remain unordered by a
common protocol. Share the coordination object and keep every relevant
concurrent access inside its protocol. Sharing the function's source text
is not sharing the mutex's identity.

### 2. Every access is atomic, yet two tickets are sold

**Problem:** both buyers load one available seat, then each stores zero and
records a sale. What guarantee was missing?

**Reasoning:** the check and conditional change were split. Atomic loads and
stores define each access; they do not make the premise remain true until
the later write. A successful CAS can be the conditional claim, or a common
mutex can cover the whole logical operation. Stronger ordering on the two
separate accesses does not merge them into one operation.

### 3. A correct count and a misleading benchmark

**Problem:** a program increments separate counters but prints the other
counter on every iteration. Does slow execution isolate false sharing?

**Reasoning:** no. The workload also contains cross-reading, formatting,
stream synchronization, and output costs. Those affect both behavior and
timing. Check counts, memory layout, and the hot-loop workload separately.
A controlled same-work/layout comparison narrows the performance claim.

### 4. The ready flag is atomic; the payload is ordinary

**Problem:** the consumer sees `ready == 1`. When is reading the ordinary
payload justified?

**Reasoning:** in the one-shot example, the consumer's acquire must observe
the release publication that followed payload initialization. That creates
the necessary ordering for the subsequent payload read. A relaxed observation
alone does not provide it. A reused payload needs an additional ownership or
lifetime protocol preventing concurrent overwrite.

### 5. An optimistic reservation fails

**Problem:** a worker saw a free ticket and later its strong CAS failed.
Was an invalid ticket issued?

**Reasoning:** failure means this attempt did not acquire ownership. Rejecting
that stale claim is how the invariant is maintained. Inspect where the
program records a successful sale; it belongs only on the successful claim
path. Another attempt or another customer operation is not automatically
replay-safe if it has external effects.

### 6. Versions match around an ordinary racy read

**Problem:** a reader races on non-atomic fields, then checks that the version
counter was unchanged. Does successful validation make the accesses legal C?

**Reasoning:** no. A post-read check cannot undo undefined behavior caused
by the accesses themselves. The teaching snapshot uses atomic payload fields
and a simple ordering model, then validates their relationship. A production
seqlock-style implementation needs an argument appropriate to its language
and platform mechanisms.

### 7. Three honest replicas split A/A/B

**Problem:** which replica should be considered untrustworthy because of
this disagreement?

**Reasoning:** none can be identified from that fact alone. Different request
arrival orders can make all three valid local decisions. The goal is one
safe accepted proposal, not a diagnosis that the minority must be lying.
To establish a protocol violation, we would need evidence of a violated rule,
such as incompatible endorsements under rules that prohibit them.

### 8. A wins in one execution; B wins in another

**Problem:** does that show conflicting commits?

**Reasoning:** not for an invariant scoped to a single initialized decision
instance. Independent runs may choose different allowed values. A conflicting
commit claim needs incompatible decisions for the same instance under the
same relevant history. A round number, a log position, and a process invocation
are not interchangeable identifiers.

### 9. Coordinator Y timed out after two votes

**Problem:** what can Y conclude about whether coordinator X collected three?

**Reasoning:** Y cannot infer X's complete observation from Y's incomplete
one. X may have the delayed vote. Recovery therefore needs rules preserving
evidence that constrains safe new decisions, rather than treating silence
as proof that no commitment exists.

### 10. An isolated old leader still accepts a matching-term job

**Problem:** is the old node's local equality test wrong?

**Reasoning:** it can correctly compare two stale local values. The missing
contract is whether that local belief is sufficient authority to produce an
accepted effect. A system may permit stale beliefs while preventing stale
actors from obtaining a valid commit or passing the resource's fencing rule.
Electing a new leader alone does not make every old belief vanish.

### 11. `poll` reports a readable client

**Problem:** may the handler assume one complete newline command will arrive
in a single `recv`?

**Reasoning:** no. Readiness concerns the stream operation, not the
application's message format. Accumulate available bytes, preserve incomplete
frames across events, and parse complete frames according to the chosen
protocol. A temporary `EAGAIN` is different from stream EOF.

### 12. Multiple active connections, one event-loop thread

**Problem:** must the request counter be atomic merely because two clients
are connected?

**Reasoning:** not if one execution thread owns all its accesses and no
asynchronous actor touches it. The connections are concurrent, but the
handlers are serialized by that architecture. Independent remote nodes still
operate concurrently; the local ownership model does not solve their agreement
problem.

## Appendix C. Building, validation, and portable use

The Markdown contains the complete explanations and teaching code. The HTML
reading copy embeds its content, styling, and navigation; it does not fetch
fonts, a diagram runtime, or a code-highlighting service. It can be opened
locally, searched, and printed from a browser. The archive also contains the
standalone files, originals, source snapshots, and validation records.

The teaching programs target Linux/WSL, C11, and POSIX threads. Barrier and
unnamed-semaphore support is not universal across platforms. The original
exploratory files are kept as submitted and can have compilation issues;
the compile audit records those instead of treating every attachment as a
ready-to-run program.

From the extracted archive, build the teaching set with `make`. It creates
21 executables and one object-file assembly probe in `build/`. The undefined
counter examples are compiled for inspection, but are not run by a blanket
“all examples must be correct” rule.

New validation for this edition compiled all 22 teaching translation units
without warnings under the recorded C11 flags. The checks covered repaired
counter totals, a unique slot claimant, explicit atomic split-update loss,
semaphore ordering, publication, inventory/reservation behavior, snapshot
accounting, ticket ownership audits, and the intended flawed voting result.
The controlled split-vote harness also passed its assertions in 20 additional
runs.

Loopback transport checks covered an incomplete line alongside a responsive
second client, coalesced commands, replies after a peer half-close, incomplete
EOF tails, overlong-line rejection, unknown commands, and reclaiming slots
across 50 sequential connections. They do not claim that the kernel was
forced to produce every possible partial-send or write-side `EAGAIN` event;
the output-offset logic is also reviewed in the listing.

Timing and retry counts in `validation/teaching-validation.json` are new
observations from this environment. They are not substituted for the learner's
screenshots or historical measurements. Assertions and successful runs provide
useful evidence under the test conditions; the invariant arguments explain
why other allowed executions should preserve the specified properties.

## Appendix D. Sources, corrections, and continuation

`F` refers to the original Systems Under Failure discussion. `C` refers to
the expanded continuation supplied for this edition. Discussion boxes are
condensed reconstructions, not invented verbatim transcript quotations.

| Material | Source range |
| --- | --- |
| Initial framework and counters | F1–14 |
| CAS, spinlocks, and assembly | F15–36 |
| Cache coherence, layout, and relay | F37–46, F63–70 |
| Memory layers, isolation, ownership, and historical debugging | F45–84 |
| Publication and inventory | F85–94 |
| Versioning, databases, seat and ticket claims | C1–56 |
| Multi-field snapshots and Byzantine assumptions | C57–68 |
| Lock-scope and deadlock repairs | C69–90 |
| Split scheduling and condition variables | C91–100 |
| Round control and reconsideration | C101–120 |
| Minority votes, prior history, and incomplete evidence | C121–144 |
| Independent machines and stale leadership | C145–156 |
| Nonblocking transport and current socket diagnosis | C157–160 |

The F snapshot was retained from the prior edition. The earlier C prefix is
retained from the prior capture, and the expanded continuation was recovered
from the newly supplied share. The combined C snapshot contains 80 learner
messages and 78 rendered assistant messages. No rendered response was
captured at C70 or C126; F likewise has no captured response at F56. Missing
responses and unrendered images/attachments have not been invented.

The separately supplied checkpoint still identified revision 0.1, whereas
the supplied manuscript contained the revision-0.2 checkpoint. This edition
preserves both supplied artifacts and advances the checkpoint using the
manuscript plus the newer conversation boundary. It does not treat the older
sidecar as evidence that the original foundations should be removed.

### Primary references for precise contracts

These references support checking terminology and API contracts independently
of the conversation. The original conversation remains the source for the
pedagogical progression and reported experiments.

- [WG14 C standards information](https://www.open-std.org/jtc1/sc22/wg14/www/standards)
  and the public [C11 working draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf):
  consult the multi-threaded execution model and `<stdatomic.h>` sections.
- [POSIX condition-variable wait specification](https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_timedwait.html):
  associated mutex, predicate reevaluation, and wait behavior.
- [Linux `poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html),
  [`recv(2)`](https://man7.org/linux/man-pages/man2/recv.2.html), and
  [`send(2)`](https://man7.org/linux/man-pages/man2/send.2.html):
  readiness and stream-operation results for the transport exercise.
- [PostgreSQL transaction-isolation documentation](https://www.postgresql.org/docs/current/transaction-iso.html):
  the qualified single-row Read Committed update example and the distinction
  from stronger isolation guarantees.

**Revision 0.3:** replaces the outline-style treatment with expanded reasoning,
worked dialogue, concrete failure traces, guided answers, and the later
coursework through nonblocking I/O. It preserves the original attempts, labels
new teaching additions, and moves the append boundary to the latest supplied
exchange. No public release or license selection is performed by this update.

<!-- COURSE_CONTINUATION_CHECKPOINT
{
  "schema": 3,
  "course": "Understanding Complex Systems",
  "revision": "0.3",
  "captured_on": "2026-10-24",
  "source_title": "Concurrency progression review",
  "source_conversation_url": "https://chatgpt.com/g/g-p-6abbdb23b2ec81918c8b65743e1546ea/c/6abd37af-0fac-83ea-b773-effcb504df27",
  "shared_snapshot_url": "https://chatgpt.com/share/6adcad23-dfbc-83ea-9862-fc5874b47452",
  "source_message_count": 158,
  "source_prompt_count": 80,
  "source_snapshot_sha256": "1454bf84555a9aaca4e9ca21516e5cc07c4b99519bd842e1828e60cefb1ae10e",
  "last_included_user_message_id": "572b94a0-4b7e-45ff-9547-8dde5aae5ef2",
  "last_included_assistant_message_id": "c4421258-8e7e-495e-b1f7-b9c9b8f828f4",
  "last_included_assistant_dom_turn": 160,
  "last_included_assistant_excerpt": " connection\nLinux waits for readable data\nLinux receives PING\nLinux responds PONG 0\nWindows receives PONG 0\n\nThen verify that two Windows clients can connect to the same node without one preventing the other from being serviced.\n\nOne particularly important idea to take away: O_NONBLOCK doesn't mean that reading and writing happen automatically or that operations wait their turn. It means the calls return instead of waiting when they cannot make progress.\n\npoll() tells you when to attempt I/O. Your code is still responsible for maintaining the connection's state between those attempts.\n\nThat's the architectural adjustment you're very close to making. And it's going to be directly useful when your three Linux nodes start exchanging votes, heartbeats, and leader announcements independently.\n\nIf you want, I can:\n\nShow how to fix the accept-and-handle sequence for non-blocking poll()\nGuide implementing a struct to track client state for non-blocking I/O\nGive a step-by-step correction plan to enable non-blocking command parsing and response",
  "last_message_timestamp": null,
  "timestamp_note": "The shared page did not expose an absolute timestamp for the final exchange; capture date is not its message date.",
  "covered_through": "Controlled split-vote scheduling; round reset and stop-on-quorum; quorum evidence under incomplete observation; LAN stale-leader demonstration; proposed peer-authorized elections; nonblocking poll server diagnosis.",
  "exercise_in_progress": "Implement reliable nonblocking command handling: accept/register, receive and buffer until a frame is complete, process once, queue/drain replies, and retire descriptors. Peer REQUEST_VOTE is not yet implemented in the source.",
  "not_completed": [
    "Peer-to-peer election implementation",
    "Durable vote/term recovery rules",
    "A replicated commit protocol preventing stale-leader conflicting work",
    "Complete Byzantine view-change/commit machinery",
    "ABA and reclamation exercises"
  ],
  "source_scope": "Foundation F plus expanded continuation C, with all 16 uploaded C files preserved and mapped. Earlier C prefix reused from the 2026-10-16 capture; new rendered exchanges recovered 2026-10-24.",
  "extraction_note": "F has 93 rendered messages (47 user, 46 assistant), no content for turn 56. C has 158 rendered messages (80 user, 78 assistant), no content for turns 70 and 126. Images and unrendered attachment content are not reconstructed.",
  "update_rule": "Resolve current manuscript, locate final included message ID or excerpt in refreshed source, append later completed lessons; track corrections to prior chapters separately. Preserve the existing document identity.",
  "verification_rule": "Treat delay-based schedules as demonstrations, not guarantees; label new runs and new implementations separately from the source conversation.",
  "sources": [
    {
      "key": "F",
      "title": "Systems Under Failure",
      "shared_snapshot_url": "https://chatgpt.com/share/6ad237fa-06b8-83ea-bd34-5c56a9008446",
      "snapshot": "sources/foundation-conversation.json",
      "message_count": 93,
      "sha256": "1c5dfdf795f580cbe5227be1dd27b62c90603276c76e4883ced4cd8ead3340c6"
    },
    {
      "key": "C",
      "title": "Concurrency progression review",
      "shared_snapshot_url": "https://chatgpt.com/share/6adcad23-dfbc-83ea-9862-fc5874b47452",
      "snapshot": "sources/continuation-expanded.json",
      "message_count": 158,
      "sha256": "1454bf84555a9aaca4e9ca21516e5cc07c4b99519bd842e1828e60cefb1ae10e"
    }
  ],
  "total_source_messages": 251,
  "total_source_prompts": 127,
  "revision_note": "Expanded explanation-first textbook, discussion reconstructions explicitly paraphrased, worked traces, code-specific commentary, review answers, offline HTML, and additional labeled teaching experiments.",
  "coverage_note": "Includes exercises offered and explained as well as learner-observed lessons. New validation outputs are labeled separately; no unshown learner run is claimed.",
  "program_count": 22,
  "chapter_count": 27,
  "last_included_source_key": "C",
  "supplied_checkpoint_note": "The separately uploaded checkpoint labels itself 0.1; the supplied manuscript contains the newer schema-2/revision-0.2 checkpoint. Both originals are preserved; this update advances the manuscript boundary to C160.",
  "editorial_additions": [
    "counter_atomic_split.c",
    "split_vote.c",
    "poll_ping.c"
  ],
  "standalone_executable_count": 21
}
END_COURSE_CONTINUATION_CHECKPOINT -->
