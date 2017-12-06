/*
  Ethereal is a UCI chess playing engine authored by Andrew Grant.
  <https://github.com/AndyGrant/Ethereal>     <andrew@grantnet.us>
  
  Ethereal is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Ethereal is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "move.h"
#include "types.h"
#include "transposition.h"

TransTable Table;
PawnTable PTable;

void initalizeTranspositionTable(TransTable * table, uint64_t megabytes){
    
    // Minimum table size is 1MB. This maps to a key of size 15.
    // We start at 16, because the loop to adjust the memory
    // size to a power of two ends with a decrement of keySize
    uint64_t keySize = 16ull;
    
    // Every bucket must be 256 bits for the following scaling
    assert(sizeof(TransBucket) == 32);

    // Scale down the table to the closest power of 2, at or below megabytes
    for (;1ull << (keySize + 5) <= megabytes << 20 ; keySize++);
    keySize -= 1;
    
    // Setup Table's data members
    table->buckets      = calloc(1ull << keySize, sizeof(TransBucket));
    table->numBuckets   = 1ull << keySize;
    table->keySize      = keySize;
    table->generation   = 0u;
    table->used         = 0u;
}

void destroyTranspositionTable(TransTable * table){
    free(table->buckets);
}

void updateTranspositionTable(TransTable * table){
    table->generation = (table->generation + 1) % 64;
}

void clearTranspositionTable(TransTable * table){
    
    unsigned int i; int j;
    TransEntry * entry;
    
    table->generation = 0u;
    table->used = 0u;
    
    for (i = 0u; i < table->numBuckets; i++){
        for (j = 0; j < BUCKET_SIZE; j++){
            entry = &(table->buckets[i].entries[j]);
            entry->value = 0;
            entry->depth = 0u;
            entry->age = 0u;
            entry->type = 0u;
            entry->bestMove = 0u;
            entry->hash16 = 0u;
        }
    }
}

TransEntry * getTranspositionEntry(TransTable * table, uint64_t hash){
    
    TransBucket * bucket = &(table->buckets[hash & (table->numBuckets - 1)]);
    int i; uint16_t hash16 = hash >> 48;
    
    #ifdef TEXEL
    return NULL;
    #endif
    
    // Search for a matching entry. Update the generation if found.
    for (i = 0; i < BUCKET_SIZE; i++){
        if (bucket->entries[i].hash16 == hash16){
            bucket->entries[i].age = table->generation;
            return &(bucket->entries[i]);
        }
    }
    
    // No entry found
    return NULL;
}

void storeTranspositionEntry(TransTable * table, int depth, int type, int value, int bestMove, uint64_t hash){
    
    // Validate Parameters
    assert(depth < MAX_DEPTH && depth >= 0);
    assert(type == PVNODE || type == CUTNODE || type == ALLNODE);
    assert(value <= MATE && value >= -MATE);
    
    TransBucket * bucket = &(table->buckets[hash & (table->numBuckets - 1)]);
    TransEntry * entries = bucket->entries;
    TransEntry * oldOption = NULL;
    TransEntry * lowDraftOption = NULL;
    TransEntry * toReplace = NULL;
    
    int i; uint16_t hash16 = hash >> 48;
    
    for (i = 0; i < BUCKET_SIZE; i++){
        
        // Found an unused entry
        if (entries[i].type == 0){
            table->used += 1;
            toReplace = &(entries[i]);
            goto Replace;
        }
        
        // Found an entry with the same hash key
        if (entries[i].hash16 == hash16){
            toReplace = &(entries[i]);
            goto Replace;
        }
        
        // Search for the lowest draft of an old entry
        if (entries[i].age != table->generation)
            if (oldOption == NULL || oldOption->depth >= entries[i].depth)
                oldOption = &(entries[i]);
        
        // Search for the lowest draft if no old entry has been found yet
        if (oldOption == NULL)
            if (lowDraftOption == NULL || lowDraftOption->depth >= entries[i].depth)
                lowDraftOption = &(entries[i]);
    }
    
    // If no old option, use the lowest draft
    toReplace = oldOption != NULL ? oldOption : lowDraftOption;
    
    Replace:
        toReplace->value    = value;
        toReplace->depth    = depth;
        toReplace->age      = table->generation;
        toReplace->type     = type;
        toReplace->bestMove = bestMove;
        toReplace->hash16   = hash16;
}

void initalizePawnTable(PawnTable * ptable){
    ptable->entries = calloc(0x10000, sizeof(PawnEntry));
}

void destoryPawnTable(PawnTable * ptable){
    free(ptable->entries);
}

PawnEntry * getPawnEntry(PawnTable * ptable, uint64_t phash){
    PawnEntry * pentry = &(ptable->entries[phash >> 48]);
    return pentry->phash == phash ? pentry : NULL;
}

void storePawnEntry(PawnTable * ptable, uint64_t phash, uint64_t passed, int mg, int eg){
    PawnEntry * pentry = &(ptable->entries[phash >> 48]);
    pentry->phash = phash;
    pentry->passed = passed;
    pentry->mg = mg;
    pentry->eg = eg;
}
