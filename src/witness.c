/* Copyright (c) 2017 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "server.h"
#include "redisassert.h"

#define MAX_WITNESS_REQUEST_SIZE 2048
//    static const int NUM_ENTRIES_PER_TABLE = 512; // Must be power of 2.
//#define WITNESS_NUM_ENTRIES_PER_TABLE 4096 // Must be power of 2.
//#define HASH_BITMASK 4095
#define WITNESS_NUM_ENTRIES_PER_TABLE 1024 // Must be power of 2.
#define HASH_BITMASK 1023
#define WITNESS_ASSOCIATIVITY 4

/**
 * Holds information to recover an RPC request in case of the master's crash
 */
struct Entry {
    bool occupied[WITNESS_ASSOCIATIVITY]; // TODO(seojin): check padding to 64-bit improves perf?
    int16_t requestSize[WITNESS_ASSOCIATIVITY];
    uint32_t keyHash[WITNESS_ASSOCIATIVITY];
    int64_t clientId[WITNESS_ASSOCIATIVITY];
    int64_t requestId[WITNESS_ASSOCIATIVITY];
    char request[WITNESS_ASSOCIATIVITY][MAX_WITNESS_REQUEST_SIZE];
    unsigned long long GcSeqNum[WITNESS_ASSOCIATIVITY]; // GcRpcCount when it arrived.
};

/**
 * Holds information of a master being witnessed. Holds recent & unsynced
 * requests to the master.
 */
struct Master {
    uint64_t id;
    bool writable;
    struct Entry table[WITNESS_NUM_ENTRIES_PER_TABLE];
    int occupiedCount;
    int gcMissedCount;
    unsigned long long totalGcRpcs;
    int totalRecordRpcs;
    int totalRejection;
    int trueCollision;
};

struct WitnessGcInfo {
    int hashIndex;
    long long clientId;
    long long requestId;
};

/* 0th index is not used. */
struct WitnessGcInfo obsoleteRpcs[50] = {{0,0,0}, };
int obsoleteRpcsSize = 0;

void addToObsoleteRpcs(int hashIndex, long long clientId, long long requestId) {
    if (obsoleteRpcsSize < 50) {
        obsoleteRpcs[obsoleteRpcsSize].hashIndex = hashIndex;
        obsoleteRpcs[obsoleteRpcsSize].clientId = clientId;
        obsoleteRpcs[obsoleteRpcsSize].requestId = requestId;
    }
    // Just ignore if this buffer is full..
}

struct Master masters[10];
time_t lastStatPrintTime = 0;
