#include <iostream>
#include <fuse.h>
#include <string>
#include <Wad.h>
#include <cstring>
#include <stdio.h>
#include <unistd.h>

// FUSE Operations:

// Getting the attributes of a file
int fs_getattr(const char* path, struct stat* stbuf) {
	Wad* wad = (Wad*)fuse_get_context()->private_data;
	
	memset(stbuf, 0, sizeof(struct stat));
	if (wad->isDirectory(path)) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else if (wad->isContent(path)) {
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;
		stbuf->st_size = wad->getSize(path);
	}
	else {
		return -ENOENT;
	}
	return 0;
}

// Creating a file
int fs_mknod(const char* path, mode_t mode, dev_t dev) {
	Wad* wad = (Wad*)fuse_get_context()->private_data;

	wad->createFile(path);

	return 0;
}

// Creating a directory
int fs_mkdir(const char* path, mode_t mode) {
	Wad* wad = (Wad*)fuse_get_context()->private_data;
	wad->createDirectory(path);

	return 0;
}

// Reading the content of a file
int fs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	Wad* wad = (Wad*)fuse_get_context()->private_data;

	if (wad->isContent(path)) {
		int bytesOut = wad->getContents(path, buf, size, offset);
		return bytesOut;
	}
	else {
		return -ENOENT;
	}
}

// Writing to a file
int fs_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	Wad* wad = (Wad*)fuse_get_context()->private_data;

	if (wad->isContent(path)) {
		int bytesIn = wad->writeToFile(path, buf, size, offset);
		return bytesIn;
	}

	return -ENOENT;
}

// Reading the contents of a directory
int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
	Wad* wad = (Wad*)fuse_get_context()->private_data;

	std::vector<std::string> directoryContent;
	if (wad->isDirectory(path)) {
		wad->getDirectory(path, &directoryContent);
		filler(buf, ".", nullptr, 0);   // Current directory
		filler(buf, "..", nullptr, 0); // Parent directory
		for (const auto& entry : directoryContent) {
			filler(buf, entry.c_str(), nullptr, 0);
		}
		return 0;
	}
	else {
		return -ENOENT;
	}

}

void fs_destroy(void* private_data) {
	Wad* wad = (Wad*)private_data;
	delete wad;
}


int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cout << "Not enough arguments" << std::endl;
		exit(EXIT_SUCCESS);
	}

	std::string wadPath = argv[argc - 2];

	if (wadPath.at(0) != '/') {
		wadPath = std::string(get_current_dir_name()) + "/" + wadPath;
	}
	Wad* wad = Wad::loadWad(wadPath);

	argv[argc - 2] = argv[argc - 1];
	argc--;

	struct fuse_operations operations = {};
	operations.getattr = fs_getattr;
	operations.readdir = fs_readdir;
	operations.mknod = fs_mknod;
	operations.mkdir = fs_mkdir;
	operations.read = fs_read;
	operations.write = fs_write;
	operations.destroy = fs_destroy;

	return fuse_main(argc, argv, &operations, wad);
}