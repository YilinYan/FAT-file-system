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

static int my_mkdir(const char *path, mode_t mode)
{
	int res;

	fprintf(FS.debug, "make dir: %s\n", path);
	fflush(FS.debug);

	return 0;
}

static int my_rmdir(const char *path)
{
	int res;

	return 0;
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
	const char* dir = path;
	char buf[NAME_LENGTH];
	directory_entry* ret = FS.root;

	if(strcmp("/", path) == 0 || path[0] == '\0')
		return ret;

	while(get_next_dir(dir, buf)) {
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

static int my_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	directory_entry* entry = find_dir_by_path(path);
	
	fprintf(FS.debug, "getattr: %sfind the entry: %s\n", 
			entry==NULL?"can't ":"can ", path);
	fflush(FS.debug);

	if(entry == NULL) return -ENOENT;

	memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_mode = 0755;
   	if(entry->flags == 1) stbuf->st_mode |= S_IFDIR;
	stbuf->st_nlink = 2;
	stbuf->st_size = entry->file_length;

	return res;
}

static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	directory_entry* dir = find_dir_by_path(path);

	fprintf(FS.debug, "readdir: %sfind the entry: %s\n", 
			dir==NULL?"can't ":"can ", path);
	fflush(FS.debug);

	if (dir == NULL) return -ENOENT;

	directory_entry* child = dir->child_first;

	for(;child != NULL; child = child->next) {
		fprintf(FS.debug, "readdir: child: %s\n", 
				child->name);
		fflush(FS.debug);

		filler(buf, child->name, NULL, 0);
	}

	return 0;
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
	directory_entry* entry = find_dir_by_path(path);
	
	fprintf(FS.debug, "open: %sfind the entry: %s\n", 
			entry==NULL?"can't ":"can ", path);
	fflush(FS.debug);

	if (entry == NULL) return -ENOENT;

	/*
	if (strcmp(path, hello_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;
*/
	return 0;
}

static int my_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
		
	directory_entry* entry = find_dir_by_path(path);
	
	fprintf(FS.debug, "read: %sfind the entry: %s\n", 
			entry==NULL?"can't ":"can ", path);
	fflush(FS.debug);

	if (entry == NULL) return -ENOENT;

	len = strlen(hello_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	} else
		size = 0;

	return size;
}

static struct fuse_operations my_oper = {
	.getattr	= my_getattr,
	.readdir	= my_readdir,
	.rmdir		= my_rmdir,
	.mkdir		= my_mkdir,
	.open		= my_open,
	.read		= my_read,
};

directory_entry* new_entry(char* name) {
	directory_entry* ret = (directory_entry*) malloc(sizeof(directory_entry));
	time_t t = time(NULL);
	strcpy(ret->name, name);
	ret->create_t = ret->modify_t = ret->access_t = t;
	ret->flags = 1;
	return ret;
}

directory_entry* new_directory_entry(char* name) {
	directory_entry* ret = new_entry(name);
	ret->child_first = new_entry(".");
	return ret;
}

char* disk_name = "mountpoint/disk"; 
static void init() {
	FS.root = new_directory_entry("");
	FS.disk = disk_name;
	FS.debug = fopen("debug", "w+");
	int fd = open(FS.disk, O_CREAT | O_RDWR);
	close(fd);
}

int main(int argc, char *argv[])
{
	init();
	return fuse_main(argc, argv, &my_oper, NULL);
}
