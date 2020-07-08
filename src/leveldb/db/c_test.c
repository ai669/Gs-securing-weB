
/* Copyright (c) 2011 The LevelDB Authors. All rights reserved.
   Use of this source code is governed by a BSD-style license that can be
   found in the LICENSE file. See the AUTHORS file for names of contributors. */

#include "leveldb/c.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

const char* phase = "";
static char dbname[200];

static void StartPhase(const char* name) {
  fprintf(stderr, "=== Test %s\n", name);
  phase = name;
}

static const char* GetTempDir(void) {
    const char* ret = getenv("TEST_TMPDIR");
    if (ret == NULL || ret[0] == '\0')
        ret = "/tmp";
    return ret;
}

#define CheckNoError(err)                                               \
  if ((err) != NULL) {                                                  \
    fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, phase, (err)); \
    abort();                                                            \
  }

#define CheckCondition(cond)                                            \
  if (!(cond)) {                                                        \
    fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, phase, #cond); \
    abort();                                                            \
  }

static void CheckEqual(const char* expected, const char* v, size_t n) {
  if (expected == NULL && v == NULL) {
    // ok
  } else if (expected != NULL && v != NULL && n == strlen(expected) &&
             memcmp(expected, v, n) == 0) {
    // ok
    return;
  } else {
    fprintf(stderr, "%s: expected '%s', got '%s'\n",
            phase,
            (expected ? expected : "(null)"),
            (v ? v : "(null"));
    abort();
  }
}

static void Free(char** ptr) {
  if (*ptr) {
    free(*ptr);
    *ptr = NULL;
  }
}

static void CheckGet(
    leveldb_t* db,
    const leveldb_readoptions_t* options,
    const char* key,
    const char* expected) {
  char* err = NULL;
  size_t val_len;
  char* val;
  val = leveldb_get(db, options, key, strlen(key), &val_len, &err);
  CheckNoError(err);
  CheckEqual(expected, val, val_len);
  Free(&val);
}

static void CheckIter(leveldb_iterator_t* iter,
                      const char* key, const char* val) {
  size_t len;
  const char* str;
  str = leveldb_iter_key(iter, &len);
  CheckEqual(key, str, len);