#include "zipper.h"
#include "unzipper.h"
#include <iostream>
#include <fstream>
#include <sys/io.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <vector>
using namespace std;
using namespace ziputils;



extern vector<string>  unzipfiles;
extern vector<string> wordrelativepath;


extern int P[100];   
string int_to_string(int i);
void createDirectory(char *path);
void zipFolder(char *docx, vector<string> unzipfile);
void unzip(char *docx);
void deletefile(vector <string> file);
void deleteFolder(char *src, vector <string>file);




char* KMP(char* A, char* B, int size);    
void InitP(char *B);         


int subfile_filter(char *path, char *filename, vector<string> keyword, bool *find_keyword ,int flag);
int masking(char *path, vector<string> word);
vector<vector<long int> >  extract_content(char *txt, int size, char *content, long int *contentLen, int flag);
int SmartArtfilter(char *path, char *filename, vector<string> word);
