#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include "mask.h"

#define FIND_NO_KEYWORD 1
#define MKDIR_FAILED -1
#define OPEN_FILE_FAILED -2

int mask(char *docx1,vector<string> keyword);

int docxmask(char **buf,int *length, vector<string> keyword);
