#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include "main.h"

#define FALSE 0
#define TRUE !FALSE

char pathname[MAXPATHLEN];

const int header_dir = 1;
const int header_file = 2;


int main(int argc, char **argv)
{

	if (getwd(pathname) == NULL) {
		printf("Error getting path\n");
		return -1;
	}
	if (argc <= 2) {
		help();
		return  -1;
	}

	int res;

	if (strcmp(argv[1], "-w") == 0)
		res = archive(argv + 2, argc - 3, argv[argc - 1]);
	else if (strcmp(argv[1], "-r") == 0)
		res = extract(argv[2]);
	else {
		help();
		res = -1;
	}

	return res;
}

void help(void)
{
	printf("Wrong usage of comand\nComand should consists of: (1) (2) (3)\n Where\n (1) = ./sarch\n (2) = [-r] for reading/extracting or [-w] for writing/archiving\n (3) = files_path\n");

}

/*
 * Archives 'n' files from 'files' array to file with 'arch' name.
 * Returns error code or 0 if it was successfull
 */
int archive(const char **files, const int n, const char *arch)
{
	printf("archive\n");
	if (access(arch, F_OK) == 0) {
		printf("Archive with name %s already exists\n", arch);
		return -2;
	}

	int arch_file = open(arch, O_WRONLY|O_CREAT);

	if (arch_file == -1) {
		printf("Can't create archive file\n");
		return -3;
	}

	write(arch_file, &n, sizeof(const int));

	int i, err_code;
	struct stat statbuf;
	char file[MAXPATHLEN];
	struct dirent **subdirfiles;

	for (i = 0; i < n; i++) {
		strcpy(file, pathname);
		strcat(file, "/");
		strcat(file, files[i]);
		printf("Current: %s\n", file);
		if (stat(file, &statbuf) == -1) {
			printf("can't read\n");
			continue;
	    }
		if (is_file(statbuf)) {
			printf("file %s\n", basename(file));
			write_file(arch_file, file);

		} else if (is_dir(statbuf)) {

			printf("folder %s\n", basename(file));
			write_dir(arch_file, basename(file));

			int count = scandir(file, &subdirfiles,
				file_select, NULL);

			if (count > 0) {
				strcat(file, "/");
				archive_subfolder(arch_file, file,
					subdirfiles, count);
			}
			int j;

			for (j = 0; j < count; j++)
				free(subdirfiles[j]);
			free(subdirfiles);

	    } else {

			printf("not found\n");
	    }
	}
	close(arch_file);
	return 0;
}

void archive_subfolder(int arch_file, char *path,
	struct dirent **files, const int n)
{
	printf("subarchive: %s\n", path);

	write(arch_file, &n, sizeof(const int));

	int i, err_code;
	struct stat statbuf;
	char file[MAXPATHLEN];
	struct dirent **subdirfiles;

	for (i = 0; i < n; i++) {
		strcpy(file, path);
		strcat(file, files[i]->d_name);
		printf("Current: %s\n", file);

		err_code = stat(file, &statbuf);
		if (err_code == -1) {
			printf("can't read, err code: %d\n", err_code);
			continue;
		}

		if (is_file(statbuf)) {
			printf("file %s\n", basename(file));
			write_file(arch_file, file);

		} else if (is_dir(statbuf)) {

			printf("folder %s\n", basename(file));
			write_dir(arch_file, basename(file));

			int count = scandir(file, &subdirfiles,
				file_select, NULL);

			if (count > 0) {
				strcat(file, "/");
				archive_subfolder(arch_file, file,
					subdirfiles, count);
			}
			int j;

			for (j = 0; j < count; j++)
				free(subdirfiles[j]);
			free(subdirfiles);
		}
	}
}

void write_file(int file_to, char *file_from)
{
	size_t fname_len = strlen(basename(file_from));
	const uint32_t file_len = file_length(file_from);

	printf("file len: %d\n", file_len);

	printf("write file %d %s\n", fname_len, basename(file_from));
	write(file_to, &header_file, sizeof(int));
	write(file_to, &fname_len, sizeof(size_t));
	write(file_to, basename(file_from), fname_len*sizeof(char));

	write(file_to, &file_len, sizeof(uint32_t));
	if (file_len > 0) {
		int file = open(file_from, O_RDONLY);
		char *buf = malloc(file_len);

		read(file, buf, file_len);
		write(file_to, buf, file_len);
		free(buf);
		close(file);
	}

}

void write_dir(int file_to, char *file_from)
{
	size_t fname_len = strlen(file_from);

	printf("write dir %d %s\n", fname_len, file_from);
	write(file_to, &header_dir, sizeof(int));
	write(file_to, &fname_len, sizeof(size_t));
	write(file_to, file_from, fname_len*sizeof(char));
}

int extract(const char *arch)
{
	int result = 0;
	char fname[MAXPATHLEN];

	strcpy(fname, pathname);
	strcat(fname, "/");
	strcat(fname, arch);
	int arch_file = open(fname, O_RDONLY);

	if (arch_file == -1) {
		printf("Can't open archive\n");
	    return -3;
	}

	result = extract_subdir(arch_file, pathname);

	close(arch_file);
	return result;
}

int extract_subdir(int arch_file, char *pathname)
{
	printf("extract subdir %s\n", pathname);
	int count;

	read(arch_file, &count, sizeof(const int));

	printf("count = %d\n", count);
	int i, mode;

	for (i = 0; i < count; ++i) {
		read(arch_file, &mode, sizeof(int));

		printf("mode = %d\n", mode);
		if (mode == header_dir) {
			if (extract_dir(arch_file, pathname) != 0)
				return -1;

			extract_subdir(arch_file, pathname);

			char dir[MAXPATHLEN];

			strcpy(dir, dirname(pathname));
			strcpy(pathname, dir);

		} else if (mode == header_file) {
			extract_file(arch_file, pathname);
		}
	}

	return 0;
}

int extract_file(int arch_file, char *path)
{
	size_t fname_len;
	uint32_t file_len;
	char *buf;
	char fname[MAXPATHLEN];

	strcpy(fname, path);
	strcat(fname, "/");

	if (!read(arch_file, &fname_len, sizeof(size_t)))
		return -1;

	buf = calloc(fname_len+1, 1);
	if (read(arch_file, buf, fname_len) < fname_len) {
		free(buf);
		return -1;
	}
	strcat(fname, buf);
	free(buf);

	if (!read(arch_file, &file_len, sizeof(uint32_t)))
		return -1;

	buf = calloc(file_len+1, 1);
	if (read(arch_file, buf, file_len) < file_len) {
		free(buf);
		return -1;
	}

	printf("extract file %s\n", fname);
	int file = open(fname, O_WRONLY|O_CREAT);

	if (file == -1) {
		printf("Can't create file\n");
		free(buf);
		return -1;
	}

	write(file, buf, file_len);
	close(file);
	free(buf);

	return 0;
}

int extract_dir(int arch_file, char *path)
{
	size_t fname_len;
	char *fname;

	strcat(path, "/");
	read(arch_file, &fname_len, sizeof(size_t));
	fname = calloc(fname_len + 1, 1);
	printf("ext fname_len %d\n", fname_len);
	if (read(arch_file, fname, fname_len) < fname_len) {
		free(fname);
		return -1;
	}
	printf("ext fname %s\n", fname);
	strcat(path, fname);
	printf("extract dir %s\n", path);
	free(fname);
	if (mkdir(path, 0700)) {
		printf("Can't create dir\n");
		return -1;
	}
	return 0;
}

/*
 * Returns file length in bytes
 */
off_t file_length(char *file)
{
	struct stat statbuf;

	if (stat(file, &statbuf) == -1)
		return -1;

	return statbuf.st_size;
}

int file_select(struct dirent *entry)
{
	if ((strcmp(entry->d_name, ".") == 0)
		|| (strcmp(entry->d_name, "..") == 0))
		return FALSE;
	else
		return TRUE;
}

int is_file(struct stat statbuf)
{
	if (S_ISREG(statbuf.st_mode) == 0)
		return FALSE;
	return TRUE;
}

int is_dir(struct stat statbuf)
{

	if (S_ISDIR(statbuf.st_mode) == 0)
		return FALSE;
	return TRUE;
}
