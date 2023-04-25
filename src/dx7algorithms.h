//
// dx7algorithms.h
//
// Yamaha DX7 Algorithms
// Copyright 2023, Bernhard Lex
// License: GPLv3+
//
// Based on info from:
// https://static.righto.com/images/dx7-alg/algorithms.jpg
//
#ifndef _dx7algorithms_h
#define _dx7algorithms_h

// ascii-diagrams exist in v1 and v2. comment-out to use ascii-diagrams v1.
#define use_ascii_v2


const char* algorithmDiagramUnicode[] = {
// UNICODE DIAGRAMS
// Algorithm #1
"      ┌──┐\n"
"     [6] │\n"
"      ├──┘\n"
"     [5]\n"
"      │\n"
"[2]  [4]\n"
" │    │\n"
"[1]  [3]\n"
" └────┘\n"
,
// Algorithm #2
"       [6]\n"
"        │\n"
"       [5]\n"
"┌──┐    │\n"
"│ [2]  [4]\n"
"└──┤    │\n"
"  [1]  [3]\n"
"   └────┘\n"
,
// Algorithm #3
"      ┌──┐\n"
"[3]  [6] │\n"
" │    ├──┘\n"
"[2]  [5]\n"
" │    │\n"
"[1]  [4]\n"
" └────┘\n"
,
// Algorithm #4
"      ┌──┐\n"
"[3]  [6] │\n"
" │    │  │\n"
"[2]  [5] │\n"
" │    │  │\n"
"[1]  [4] │\n"
" │    ├──┘\n"
" └────┘\n"
,
// Algorithm #5
"           ┌──┐\n"
"[2]  [4]  [6] │\n"
" │    │    ├──┘\n"
"[1]  [3]  [5]\n"
" └────┴────┘\n"
,
// Algorithm #6
"           ┌──┐\n"
"[2]  [4]  [6] │\n"
" │    │    │  │\n"
"[1]  [3]  [5] │\n"
" │    │    ├──┘\n"
" └────┴────┘\n"
,
// Algorithm #7
"          ┌──┐\n"
"         [6] │\n"
"          ├──┘\n"
"[2]  [4] [5]\n"
" │    │ ╱\n"
"[1]  [3]\n"
" └────┘\n"
,
// Algorithm #8
"           [6]\n"
"     ┌──┐   │\n"
"[2]  │ [4] [5]\n"
" │   └──┤ ╱\n"
"[1]    [3]\n"
" └──────┘\n"
,
// Algorithm #9
"           [6]\n"
"┌──┐        │\n"
"│ [2]  [4] [5]\n"
"└──┤    │ ╱\n"
"  [1]  [3]\n"
"   └────┘\n"
,
// Algorithm #10
"          ┌──┐\n"
"         [3] │\n"
"          ├──┘\n"
"[5] [6]  [2]\n"
"   ╲ │    │\n"
"    [4]  [1]\n"
"     └────┘\n"
,
// Algorithm #11
"           [3]\n"
"     ┌──┐   │\n"
"[5] [6] │  [2]\n"
"   ╲ ├──┘   │\n"
"    [4]    [1]\n"
"     └──────┘\n"
,
// Algorithm #12
"              ┌──┐\n"
"[4] [5] [6]  [2] │\n"
"   ╲ │ ╱      ├──┘\n"
"    [3]      [1]\n"
"     └────────┘\n"
,
// Algorithm #13
"         ┌──┐\n"
"[4] [5] [6] │  [2]\n"
"   ╲ │ ╱ └──┘   │\n"
"    [3]        [1]\n"
"     └──────────┘\n"
,
// Algorithm #14
"       ┌──┐\n"
"  [5] [6] │\n"
"     ╲ ├──┘\n"
"[2]   [4]\n"
" │     │\n"
"[1]   [3]\n"
" └─────┘\n"
,
// Algorithm #15
"     [5] [6]\n"
"┌──┐    ╲ │\n"
"│ [2]    [4]\n"
"└──┤      │\n"
"  [1]    [3]\n"
"   └──────┘\n"
,
// Algorithm #16
"         ┌──┐\n"
"    [4] [6] │\n"
"     │   ├──┘\n"
"[2] [3] [5]\n"
"   ╲ │ ╱\n"
"    [1]\n"
"     │\n"
,
// Algorithm #17
"      [4] [6]\n"
"┌──┐   │   │\n"
"│ [2] [3] [5]\n"
"└──┘ ╲ │ ╱\n"
"      [1]\n"
"       │\n"
,
// Algorithm #18
"          [6]\n"
"           │\n"
"          [5]\n"
"      ┌─┐  │\n"
"[2]  [3]│ [4]\n"
"   ╲  ├─┘╱\n"
"    ╲ │ ╱\n"
"     [1]\n"
"      │\n"
,
// Algorithm #19
"[3]\n"
" │    ┌──┐\n"
"[2]  [6] │\n"
" │    │ ╲┘\n"
"[1]  [4] [5]\n"
" └────┴───┘\n"
,
// Algorithm #20
" ┌──┐\n"
"[3] │   [5] [6]\n"
" │ ╲┘      ╲ │\n"
"[1] [2]     [4]\n"
" └───┴───────┘\n"
,
// Algorithm #21
" ┌──┐\n"
"[3] │   [6]\n"
" │ ╲┘    │ ╲\n"
"[1] [2] [4] [5]\n"
" └───┴───┴───┘\n"
,
// Algorithm #22
"          ┌──┐\n"
"[2]      [6] │\n"
" │      ╱ │ ╲┘\n"
"[1]  [3] [4] [5]\n"
" └────┴───┴───┘\n"
,
// Algorithm #23
"           ┌──┐\n"
"     [3]  [6] │\n"
"      │    │ ╲┘\n"
"[1]  [2]  [4] [5]\n"
" └────┴────┴───┘\n"
,
// Algorithm #24
"               ┌──┐\n"
"              [6] │\n"
"             ╱ │ ╲┘\n"
"[1]  [2]  [3] [4] [5]\n"
" └────┴────┴───┴───┘\n"
,
// Algorithm #25
"                ┌──┐\n"
"               [6] │\n"
"                │ ╲┘\n"
"[1]  [2]  [3]  [4] [5]\n"
" └────┴────┴────┴───┘\n"
,
// Algorithm #26
"               ┌──┐\n"
"     [3]  [5] [6] │\n"
"      │      ╲ ├──┘\n"
"[1]  [2]      [4]\n"
" └────┴────────┘\n"
,
// Algorithm #27
"      ┌──┐\n"
"     [3] │  [5] [6]\n"
"      ├──┘     ╲ │\n"
"[1]  [2]        [4]\n"
" └────┴──────────┘\n"
,
// Algorithm #28
"      ┌──┐\n"
"     [5] │\n"
"      ├──┘\n"
"[2]  [4]\n"
" │    │\n"
"[1]  [3]  [6]\n"
" └────┴────┘\n"
,
// Algorithm #29
"                ┌──┐\n"
"          [4]  [6] │\n"
"           │    ├──┘\n"
"[1]  [2]  [3]  [5]\n"
" └────┴────┴────┘\n"
,
// Algorithm #30
"           ┌──┐\n"
"          [5] │\n"
"           ├──┘\n"
"          [4]\n"
"           │\n"
"[1]  [2]  [3]  [6]\n"
" └────┴────┴────┘\n"
,
// Algorithm #31
"                     ┌──┐\n"
"                    [6] │\n"
"                     ├──┘\n"
"[1]  [2]  [3]  [4]  [5]\n"
" └────┴────┴────┴────┘\n"
,
// Algorithm #32
"                          ┌──┐\n"
"[1]  [2]  [3]  [4]  [5]  [6] │\n"
" │    │    │    │    │    ├──┘\n"
" └────┴────┴────┴────┴────┘\n"
};


const char* algorithmDiagramAscii[] = {
// ASCII DIAGRAMS
// Algorithm #1
"      +--+\n"
"     [6] |\n"
"      |--+\n"
"     [5]\n"
"      |\n"
"[2]  [4]\n"
" |    |\n"
"[1]  [3]\n"
" |    |\n"
" +----+\n"
,
// Algorithm #2
"       [6]\n"
"        |\n"
"       [5]\n"
"+--+    |\n"
"| [2]  [4]\n"
"+--|    |\n"
"  [1]  [3]\n"
"   |    |\n"
"   +----+\n"
,
// Algorithm #3
"      +--+\n"
"[3]  [6] |\n"
" |    |--+\n"
"[2]  [5]\n"
" |    |\n"
"[1]  [4]\n"
" |    |\n"
" +----+\n"
,
// Algorithm #4
"      +--+\n"
"[3]  [6] |\n"
" |    |  |\n"
"[2]  [5] |\n"
" |    |  |\n"
"[1]  [4] |\n"
" |    |--+\n"
" +----+\n"
,
// Algorithm #5
"           +--+\n"
"[2]  [4]  [6] |\n"
" |    |    |--+\n"
"[1]  [3]  [5]\n"
" |    |    |\n"
" +----+----+\n"
,
// Algorithm #6
"           +--+\n"
"[2]  [4]  [6] |\n"
" |    |    |  |\n"
"[1]  [3]  [5] |\n"
" |    |    |--+\n"
" +----+----+\n"
,
// Algorithm #7
"          +--+\n"
"         [6] |\n"
"          |--+\n"
"[2]  [4] [5]\n"
" |    | /\n"
"[1]  [3]\n"
" |    | \n"
" +----+\n"
,
// Algorithm #8
"           [6]\n"
"     +--+   |\n"
"[2]  | [4] [5]\n"
" |   +--| /\n"
"[1]    [3]\n"
" |      | \n"
" +------+\n"
,
// Algorithm #9
"           [6]\n"
"+--+        |\n"
"| [2]  [4] [5]\n"
"+--|    | /\n"
"  [1]  [3]\n"
"   |    | \n"
"   +----+\n"
,
// Algorithm #10
"          +--+\n"
"         [3] |\n"
"          |--+\n"
"[5] [6]  [2]\n"
"   \\ |    |\n"
"    [4]  [1]\n"
"     |    |\n"
"     +----+\n"
,
// Algorithm #11
"           [3]\n"
"     +--+   |\n"
"[5] [6] |  [2]\n"
"   \\ |--+   |\n"
"    [4]    [1]\n"
"     |      |\n"
"     +------+\n"
,
// Algorithm #12
"              +--+\n"
"[4] [5] [6]  [2] |\n"
"   \\ | /      |--+\n"
"    [3]      [1]\n"
"     |        |\n"
"     +--------+\n"
,
// Algorithm #13
"         +--+\n"
"[4] [5] [6] |  [2]\n"
"   \\ | / +--+   |\n"
"    [3]        [1]\n"
"     |          |\n"
"     +----------+\n"
,
// Algorithm #14
"       +--+\n"
"  [5] [6] |\n"
"     \\ |--+\n"
"[2]   [4]\n"
" |     |\n"
"[1]   [3]\n"
" |     |\n"
" +-----+\n"
,
// Algorithm #15
"     [5] [6]\n"
"+--+    \\ |\n"
"| [2]    [4]\n"
"+--|      |\n"
"  [1]    [3]\n"
"   |      |\n"
"   +------+\n"
,
// Algorithm #16
"         +--+\n"
"    [4] [6] |\n"
"     |   |--+\n"
"[2] [3] [5]\n"
"   \\ | / \n"
"    [1]\n"
"     |\n"
//"     +\n"
,
// Algorithm #17
"      [4] [6]\n"
"+--+   |   |\n"
"| [2] [3] [5]\n"
"+--+ \\ | / \n"
"      [1]\n"
"       |\n"
//"       +\n"
,
// Algorithm #18
"          [6]\n"
"           |\n"
"          [5]\n"
"      +-+  |\n"
"[2]  [3]| [4]\n"
"   \\  |-+/\n"
"    \\ | /\n"
"     [1]\n"
"      |\n"
//"      +\n"
,
// Algorithm #19
"[3]\n"
" |    +--+\n"
#ifdef use_ascii_v2
"[2]  [6] |\n"
" |    | \\+\n"
#else
"[2]  [6] /\n"
" |    | \\\n"
#endif
"[1]  [4] [5]\n"
" |    |   |\n"
" +----+---+\n"
,
// Algorithm #20
" +--+\n"
#ifdef use_ascii_v2
"[3] |   [5] [6]\n"
" | \\+      \\ |\n"
#else
"[3] /   [5] [6]\n"
" | \\       \\ |\n"
#endif
"[1] [2]     [4]\n"
" |   |       |\n"
" +---+-------+\n"
,
// Algorithm #21
" +--+\n"
#ifdef use_ascii_v2
"[3] |   [6]\n"
" | \\+    | \\\n"
#else
"[3] /   [6]\n"
" | \\     | \\\n"
#endif
"[1] [2] [4] [5]\n"
" |   |   |   |\n"
" +---+---+---+\n"
,
// Algorithm #22
"          +--+\n"
#ifdef use_ascii_v2
"[2]      [6] |\n"
" |      / | \\+\n"
#else
"[2]      [6] /\n"
" |      / | \\\n"
#endif
"[1]  [3] [4] [5]\n"
" |    |   |   |\n"
" +----+---+---+\n"
,
// Algorithm #23
"           +--+\n"
#ifdef use_ascii_v2
"     [3]  [6] |\n"
"      |    | \\+\n"
#else
"     [3]  [6] /\n"
"      |    | \\\n"
#endif
"[1]  [2]  [4] [5]\n"
" |    |    |   |\n"
" +----+----+---+\n"
,
// Algorithm #24
"               +--+\n"
#ifdef use_ascii_v2
"              [6] |\n"
"             / | \\+\n"
#else
"              [6] /\n"
"             / | \\\n"
#endif
"[1]  [2]  [3] [4] [5]\n"
" |    |    |   |   |\n"
" +----+----+---+---+\n"
,
// Algorithm #25
"                +--+\n"
#ifdef use_ascii_v2
"               [6] |\n"
"                | \\+\n"
#else
"               [6] /\n"
"                | \\\n"
#endif
"[1]  [2]  [3]  [4] [5]\n"
" |    |    |    |   |\n"
" +----+----+----+---+\n"
,
// Algorithm #26
"               +--+\n"
"     [3]  [5] [6] |\n"
"      |      \\ |--+\n"
"[1]  [2]      [4]\n"
" |    |        |\n"
" +----+--------+\n"
,
// Algorithm #27
"      +--+\n"
"     [3] |  [5] [6]\n"
"      |--+     \\ |\n"
"[1]  [2]        [4]\n"
" |    |          |\n"
" +----+----------+\n"
,
// Algorithm #28
"      +--+\n"
"     [5] |\n"
"      |--+\n"
"[2]  [4]\n"
" |    |\n"
"[1]  [3]  [6]\n"
" |    |    |\n"
" +----+----+\n"
,
// Algorithm #29
"                +--+\n"
"          [4]  [6] |\n"
"           |    |--+\n"
"[1]  [2]  [3]  [5]\n"
" |    |    |    |\n"
" +----+----+----+\n"
,
// Algorithm #30
"           +--+\n"
"          [5] |\n"
"           |--+\n"
"          [4]\n"
"           |\n"
"[1]  [2]  [3]  [6]\n"
" |    |    |    |\n"
" +----+----+----+\n"
,
// Algorithm #31
"                     +--+\n"
"                    [6] |\n"
"                     |--+\n"
"[1]  [2]  [3]  [4]  [5]\n"
" |    |    |    |    |\n"
" +----+----+----+----+\n"
,
// Algorithm #32
"                          +--+\n"
"[1]  [2]  [3]  [4]  [5]  [6] |\n"
" |    |    |    |    |    |--+\n"
" +----+----+----+----+----+\n"
};


#endif