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

static const char* get_next_dir(const char* path, char* buf) {
	// not /... or is / or is \0
	if(path[0] != '/' || path[1] == '\0'
			|| path[0] == '\0') return NULL;
	
	const char* next = path + 1;
	while(*next != '/' && *next != '\0') ++next;
	strncpy(buf, path+1, next-path);
  	buf[next-path-1] = '\0';
	path = next;
	
	return path;	
}

static directory_entry* find_dir_by_path(const char* path) {
	const char* dir = path;
	char buf[NAME_LENGTH];
	directory_entry* ret = FS.root;

	if(strcmp("/", path) == 0 || path[0] == '\0')
		return ret;

	fprintf(FS.debug, "\tfind dir by path: %s\n", path);
	fflush(FS.debug);

	while((dir = get_next_dir(dir, buf)) != NULL) {
		fprintf(FS.debug, "\t%s\t%s", dir, buf);
		fflush(FS.debug);

		int found = 0;
		directory_entry* child = ret->child_first;
		for(; child; child=child->next) {
			if(strcmp(child->name, buf) == 0) {
				ret = child;
				// if .. , redirect to real dir 
				if(strcmp(buf, "..") == 0) 
					ret = child->real_dir;
				
				fprintf(FS.debug, "\tgot it\n");
				fflush(FS.debug);
				
				found = 1;
				break;
			}
		}
		if(found == 1) continue;
		
		fprintf(FS.debug, "\tungot it\n");
		fflush(FS.debug);		

		return NULL;
	}

	return ret;
}

static int is_empty_dir(directory_entry* dir) {
	directory_entry* child = dir->child_first;
	for(; child; child = child->next) {
		if(strcmp(child->name, "..") &&
				strcmp(child->name, "."))
			return 0;
	}
	return 1;
}

static directory_entry* find_dir_parent(directory_entry* dir) {
	directory_entry* child = dir->child_first;
	for(; child; child = child->next) {
		if(strcmp(child->name, "..") == 0)
			return child->real_dir;
	}
	return NULL;
}

static void rm_sub_dir(directory_entry* dir, directory_entry* child) {
	directory_entry* before = dir->child_first;
	while(before->next != child && before)
		before = before->next;
	if(!before) return;
	before->next = child->next;
}

static void rm_dir(directory_entry* dir, int flag) {
	if(!dir) return;

	if(flag) rm_dir(dir->child_first, 0);
	else rm_dir(dir->next, 0);
	free(dir);
}

static int my_rmdir(const char *path)
{
	int res;
	directory_entry* entry = find_dir_by_path(path);
	
	fprintf(FS.debug, "rmdir: %sfind the entry: %s\n", 
			entry==NULL?"can't ":"can ", path);
	fflush(FS.debug);

	if(entry == NULL) return -ENOENT;
	if(entry == FS.root) return -EACCES;
	if(!is_empty_dir(entry)) return -ENOTEMPTY;

	directory_entry* dir = find_dir_parent(entry);
	
	fprintf(FS.debug, "rmdir: %sfind dir parent: %s\n", 
			dir==NULL?"can't ":"can ", dir->name);
	fflush(FS.debug);

	if(dir == NULL) return -ENOENT; 
	rm_sub_dir(dir, entry);
	rm_dir(entry, 1);

	return 0;
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
	if(entry->flags == 1) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} 
	else {
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		stbuf->st_size = entry->file_length;
	}

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

directory_entry* new_entry(char* name) {
	directory_entry* ret = (directory_entry*) malloc(sizeof(directory_entry));
	time_t t = time(NULL);
	strcpy(ret->name, name);
	ret->create_t = ret->modify_t = ret->access_t = t;
	ret->flags = 1;
	return ret;
}

directory_entry* new_file(char* name) {
	directory_entry* file = new_entry(name);
	file->flags = 0;
	return file;
}

directory_entry* new_directory_entry(char* name) {
	directory_entry* ret = new_entry(name);
	ret->child_first = new_entry(".");
	return ret;
}

static void get_parent_path(const char *path, char *parent, char *name) {
	const char *next = path;
	const char *i = path;
	for(; *i != '\0'; ++i)
		if(*i == '/') next = i;
	// path is /xxx
	if(next == path) {
		parent[0] = '/';
		parent[1] = '\0';
	}
	// path is /xxx/xxx/...
	else {
		fprintf(FS.debug, "get parent path: %s %d\n", 
				path, next-path);
		fflush(FS.debug);

		strncpy(parent, path, next-path);
		parent[next-path] = '\0';
	}
	//get the new dir name
	strncpy(name, next + 1, i-next);
	name[i-next-1] = '\0';
}

static void add_sub_dir(directory_entry* dir, directory_entry* child) {
	fprintf(FS.debug, "add sub dir: %s -> %s\n", 
			dir->name, child->name);
	fflush(FS.debug);
	
	// add child to dir's last child
	directory_entry* last = dir->child_first;
	while(last->next) last = last->next;
	last->next = child;

	// add .. to child, connect .. to dir
	last = child->child_first;
	while(last->next) last = last->next;
	last->next = new_entry("..");
	last->next->real_dir = dir;
}

static void add_sub_file(directory_entry* dir, directory_entry* child) {
	fprintf(FS.debug, "add sub file: %s -> %s\n", 
			dir->name, child->name);
	fflush(FS.debug);
	
	// add child to dir's last child
	directory_entry* last = dir->child_first;
	while(last->next) last = last->next;
	last->next = child;
}

static int my_mkdir(const char *path, mode_t mode) {
	int res;

	fprintf(FS.debug, "mkdir: %s\n", path);
	fflush(FS.debug);

	char parent[256], name[NAME_LENGTH];
	get_parent_path(path, parent, name);

	directory_entry* dir = find_dir_by_path(parent);
		
	fprintf(FS.debug, "mkdir: %sfind the parent: %s\n", 
			dir==NULL?"can't ":"can ", parent);
	fflush(FS.debug);
	if (dir == NULL) return -ENOENT;

	directory_entry* new_dir = new_directory_entry(name);
	add_sub_dir(dir, new_dir);

	return 0;
}

static int my_open(const char *path, struct fuse_file_info *fi) {
	directory_entry* entry = find_dir_by_path(path);
	
	fprintf(FS.debug, "open: %sfind the entry: %s\n", 
			entry==NULL?"can't ":"can ", path);
	fflush(FS.debug);

	if (entry == NULL) return -ENOENT;
	if (entry && entry->flags == 1) return -EISDIR;

	return 0;
}

static int my_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi) {
	size_t len;
	(void) fi;
		
	directory_entry* entry = find_dir_by_path(path);
	
	fprintf(FS.debug, "read: %sfind the entry: %s\n", 
			entry==NULL?"can't ":"can ", path);
	fflush(FS.debug);
	if (entry == NULL) return -ENOENT;
	if (entry->flags == 1) return -EISDIR;

	strncpy(buf, "hello, this is yilin.\n", size);

	return size;
}

static int my_write (const char *path, const char *buf, size_t size, 
		off_t offset, struct fuse_file_info *fi) {
	directory_entry* file = find_dir_by_path(path);
	
	fprintf(FS.debug, "write: %sfind the file: %s\n", 
			file==NULL?"can't ":"can ", path);
	fflush(FS.debug);
	if (file == NULL) return -ENOENT;
	if (file->flags == 1) return -EISDIR;

	fprintf(FS.debug, "write: length: %d\n", size);
	fflush(FS.debug);

	file->file_length = size;

	return 0;
}

static int my_create (const char *path, mode_t mode, 
		struct fuse_file_info *fi) {
	char parent[256], name[NAME_LENGTH];
	get_parent_path(path, parent, name);

	directory_entry* entry = find_dir_by_path(parent);
		
	fprintf(FS.debug, "create: %sfind the parent: %s\n", 
			entry==NULL?"can't ":"can ", parent);
	fflush(FS.debug);
	if (entry == NULL) return -ENOENT;
	if (entry && entry->flags == 0) return -ENOENT;

	directory_entry* file = new_file(name);
	add_sub_file(entry, file);
	
	return 0;
}

static int my_utimens(const char *path, const struct timespec tv[2]) {
	directory_entry* dir = find_dir_by_path(path);
		
	fprintf(FS.debug, "utimens: %sfind the entry: %s\n", 
			dir==NULL?"can't ":"can ", path);
	fflush(FS.debug);
	if (dir == NULL) return -ENOENT;

	dir->access_t = tv[0].tv_sec;
	dir->modify_t = tv[1].tv_sec;

	return 0;
}

static int my_mknod(const char * path, mode_t mode, dev_t dev) {
	fprintf(FS.debug, "mknod: %s\n", path);
   	fflush(FS.debug);	
	return 0;
}

static int my_chmod(const char * path, mode_t mode) {
	fprintf(FS.debug, "chmod: %s\n", path);
   	fflush(FS.debug);	
	return 0;
}

static int my_chown(const char * path, uid_t uid, gid_t gid) {
	fprintf(FS.debug, "chown: %s\n", path);
   	fflush(FS.debug);	
	return 0;
}

static int my_truncate(const char * path, off_t off) {
	fprintf(FS.debug, "truncate: %s\n", path);
   	fflush(FS.debug);	
	/*
	directory_entry* file = find_dir_by_path(path);
	if (file == NULL) return -ENOENT;
	if (file->flags == 1) return -EISDIR;

	file->file_length = off;
	*/
	return 0;
}

char* disk_name = "mountpoint/disk"; 
static void init() {
	FS.root = new_directory_entry("");
	FS.disk = disk_name;
	FS.debug = fopen("debug", "w+");
	int fd = open(FS.disk, O_CREAT | O_RDWR);
	close(fd);
}

static struct fuse_operations my_oper = {
	.getattr	= my_getattr,
	.readdir	= my_readdir,
	.rmdir		= my_rmdir,
	.mkdir		= my_mkdir,
	.mknod		= my_mknod,
	.utimens	= my_utimens,
	.chmod		= my_chmod,
	.chown		= my_chown,
	.truncate	= my_truncate,
	.create		= my_create,
	.open		= my_open,
	.write		= my_write,
	.read		= my_read,
	//	.unlink		= my_unlink,
};

int main(int argc, char *argv[])
{
	init();
	return fuse_main(argc, argv, &my_oper, NULL);
}
