#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "myfs.h"

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

static int hello_mkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int hello_rmdir(const char *path)
{
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, hello_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
	} else
		res = -ENOENT;

	return res;
}


static int get_next_dir(const char* path, char* buf) {
	if(path[0] != '/' || path[1] == '\0') return 0;
	
	const char* next = path + 1;
	while(*next != '/' && *next != '\0') ++next;
	strncpy(buf, path+1, next-path);
  	buf[next-path-1] = '\0';
	path = next;
	return 1;	
}

static directory_entry* find_dir_by_path(const char* path) {
	fprintf(FS.debug, "find dir by name: %s\n", path);
	fflush(FS.debug);

	const char* dir = path;
	char buf[NAME_LENGTH];
	directory_entry* ret = FS.root;

	if(strcmp("/", path) == 0 || path[0] == '\0')
		return ret;

	while(get_next_dir(dir, buf)) {
		fprintf(FS.debug, "subdir name: %s\n", buf);
		fflush(FS.debug);

		int found = 0;
		directory_entry* child = ret->child_first;
		for(; child; child=child->next) {
			if(strcmp(child->name, buf)) {
				ret = child;
				found = 1;
				break;
			}
		}
		if(found == 1) continue;
		return NULL;
	}

	return ret;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	find_dir_by_path(path);

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, hello_path + 1, NULL, 0);

	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	
	find_dir_by_path(path);

	if (strcmp(path, hello_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	if(strcmp(path, hello_path) != 0)
		return -ENOENT;

	len = strlen(hello_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	} else
		size = 0;

	return size;
}

static struct fuse_operations hello_oper = {
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
	.read		= hello_read,
	.rmdir		= hello_rmdir,
	.mkdir		= hello_mkdir,
};

void directory_entry_init(directory_entry* entry) {
	time_t t = time(NULL);
	entry->create_t = entry->modify_t = entry->access_t = t;
}


char* disk_name = "mountpoint/disk"; 
static void init() {
	FS.root = (directory_entry*) malloc(sizeof(directory_entry));
	directory_entry_init(FS.root);
	FS.disk = disk_name;
	FS.debug = fopen("debug", "w+");
	int fd = open(FS.disk, O_CREAT | O_RDWR);
	close(fd);
}

int main(int argc, char *argv[])
{
	init();
	return fuse_main(argc, argv, &hello_oper, NULL);
}
