#ifndef CORE_H
#define CORE_H
#include <stdio.h>
#define ITEMS 6
#define COLS 76
#define ROWS 20
#define PAGES 256
typedef struct {char id[9],kind[8],label[41],path[64];} Item;
typedef struct {FILE *f;long offsets[PAGES];int count,current,more;} Pager;
int safe_path(const char *s);
int manifest(FILE *f,Item out[ITEMS]);
int read_page(FILE *f,long start,char lines[ROWS][COLS+1],long *next);
void pager_init(Pager *p,FILE *f);
int pager_load(Pager *p,char lines[ROWS][COLS+1]);
int pager_action(Pager *p,int key);
#endif

