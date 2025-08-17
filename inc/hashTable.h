#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 1000

// Struct for a hash table entry (key-value pair)
typedef struct Entry { // in our example is the hash of an already calculated filename
    char *key;          // filename
    char *value;        // hashcode
    struct Entry *next;
} Entry;

// Struct for the hash table
typedef struct {
    Entry *table[TABLE_SIZE];
} HashTable;

// Create a new hash table
HashTable *create_hash_table();

// Insert a key-value pair into the hash table
void hash_table_insert(HashTable *ht, const char *key, const char *value);

// Get the value associated with a key
char *hash_table_get(HashTable *ht, const char *key);

// Remove a key-value pair by key
void hash_table_remove(HashTable *ht, const char *key);

// Free the entire hash table
void free_hash_table(HashTable *ht);

#endif // HASH_TABLE_H
