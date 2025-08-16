#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/hashTable.h"

#define TABLE_SIZE 10000

// Simple hash function
unsigned int hash(const char *key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31) + *key;
        key++;
    }
    return hash % TABLE_SIZE;
}

// Initialize hash table
HashTable *create_hash_table() {
    HashTable *ht = malloc(sizeof(HashTable));
    if (!ht) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < TABLE_SIZE; i++) {
        ht->table[i] = NULL;
    }
    return ht;
}

// Insert a key-value pair into the hash table
void hash_table_insert(HashTable *ht, const char *key, const char *value) {
    unsigned int index = hash(key);
    Entry *entry = ht->table[index];

    // Check if key already exists
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            free(entry->value);
            entry->value = strdup(value);
            return;
        }
        entry = entry->next;
    }

    // Insert new entry at the head of the list
    Entry *new_entry = malloc(sizeof(Entry));
    if (!new_entry) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    new_entry->key = strdup(key);
    new_entry->value = strdup(value);
    new_entry->next = ht->table[index];
    ht->table[index] = new_entry;
}

// Get value by key from the hash table
char *hash_table_get(HashTable *ht, const char *key) {
    unsigned int index = hash(key);
    Entry *entry = ht->table[index];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0)
            return entry->value;
        entry = entry->next;
    }
    return NULL; // Key not found
}

// Remove a key-value pair from the hash table
void hash_table_remove(HashTable *ht, const char *key) {
    unsigned int index = hash(key);
    Entry *entry = ht->table[index];
    Entry *prev = NULL;

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (prev == NULL)
                ht->table[index] = entry->next;
            else
                prev->next = entry->next;

            free(entry->key);
            free(entry->value);
            free(entry);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

// Free the hash table and all its entries
void free_hash_table(HashTable *ht) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Entry *entry = ht->table[i];
        while (entry != NULL) {
            Entry *next = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
    }
    free(ht);
}