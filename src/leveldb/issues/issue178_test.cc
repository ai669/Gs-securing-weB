// Copyright (c) 2013 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Test for issue 178: a manual compaction causes deleted data to reappear.
#include <iostream>
#include <sstream>
#include <cstdlib>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include "util/testharness.h"

namespace {

const int kNumKeys = 1100000;

std::string Key1(int i) {
  char buf[100];
  snprintf(buf, sizeof(buf), "my_key_%d", i);
  return buf;
}

std::string Key2(int i) {
  return Key1(i) + "_xxx";
}

class Issue178 { };

TEST(Issue178, Test) {
  // Get rid of any state from an old run.
  std::string dbpath = leveldb::test::TmpDir() + "/leveldb_cbug_test";
  DestroyDB(dbpath, leveldb::Options());

  // Open database.  Disable compression since it affects the creation
  // of layers and the code below is trying to test against a very
  // specific scenario.
  leveldb::DB* db;
  leveldb::Options db_options;
  db_options.create_if_missing = true;
  db_options.compression = leveldb::kNoCompression;
  ASSERT_OK(leveldb::DB::Open(db_options, dbpath, &db));

  // create first key range
  leveldb::WriteBatch batch;
  for (size_t i = 0; i < kNumKeys; i++) {
    batch.Put(Key1(i), "value for range 1 ke