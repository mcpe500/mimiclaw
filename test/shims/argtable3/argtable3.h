#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* arg_lit */
struct arg_lit { const char *shortopts; const char *longopts; const char *glossary; int count; int hdr; };
/* arg_str */
struct arg_str { const char *shortopts; const char *longopts; const char *datatype; const char *glossary; int count; int hdr; int smax; char **sval; };
/* arg_int */
struct arg_int { const char *shortopts; const char *longopts; const char *datatype; const char *glossary; int count; int hdr; int smax; int *ival; };
/* arg_end */
struct arg_end { const char *shortopts; const char *longopts; const char *datatype; const char *glossary; int count; int hdr; int smax; int *ival; int *error; void **parent; const char **argval; };
/* Constructors */
static inline struct arg_lit *arg_lit0(const char *s, const char *l, const char *g) { struct arg_lit *a = calloc(1, sizeof(*a)); if(a) { a->shortopts=s; a->longopts=l; a->glossary=g; } return a; }
static inline struct arg_lit *arg_lit1(const char *s, const char *l, const char *g) { return arg_lit0(s,l,g); }
static inline struct arg_str *arg_str0(const char *s, const char *l, const char *d, const char *g) { struct arg_str *a = calloc(1, sizeof(*a)); if(a) { a->shortopts=s; a->longopts=l; a->datatype=d; a->glossary=g; a->smax=1; a->sval=calloc(2,sizeof(char*)); } return a; }
static inline struct arg_str *arg_str1(const char *s, const char *l, const char *d, const char *g) { return arg_str0(s,l,d,g); }
static inline struct arg_int *arg_int0(const char *s, const char *l, const char *d, const char *g) { struct arg_int *a = calloc(1, sizeof(*a)); if(a) { a->shortopts=s; a->longopts=l; a->datatype=d; a->glossary=g; a->smax=1; a->ival=calloc(2,sizeof(int)); } return a; }
static inline struct arg_int *arg_int1(const char *s, const char *l, const char *d, const char *g) { return arg_int0(s,l,d,g); }
static inline struct arg_end *arg_end(int maxerrors) { struct arg_end *a = calloc(1, sizeof(*a)); (void)maxerrors; return a; }
/* Parse/print */
static inline int arg_parse(int argc, char **argv, void **argtable) { (void)argc; (void)argv; (void)argtable; return 0; }
static inline void arg_print_errors(FILE *fp, struct arg_end *end, const char *progname) { (void)fp; (void)end; (void)progname; }
static inline void arg_print_glossary(FILE *fp, void **argtable, const char *fmt) { (void)fp; (void)argtable; (void)fmt; }
static inline void arg_freetable(void **argtable, size_t n) { (void)argtable; (void)n; }
