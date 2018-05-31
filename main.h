#ifndef MAIN_H
#define MAIN_H
#include <sys/stat.h>
#include <dirent.h>
off_t file_length(char *file);
void help(void);
int archive(const char **files, const int n, const char *arch);
void archive_subfolder(int arch_name, char *path,
	struct dirent **files, int count);
void write_file(int file_to, char *file_from);
void write_dir(int file_to, char *file_from);
int extract(const char *arch);
int extract_subdir(int arch_file, char *pathname);
int extract_file(int arch_file, char *path);
int extract_dir(int arch_file, char *path);
int file_select(struct dirent *entry);
int is_file(struct stat statbuf);
int is_dir(struct stat statbuf);
int extract(const char *arch);
#endif
