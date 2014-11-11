// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference counting via hash table.
// unlike most hash tables, the hash is the full key;
// this is possible because it is just the object address with low zero bits shifted off.

#include "08-fmt.h"

