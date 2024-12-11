#pragma once
#include <iostream>
#include <vector>
#include <stack>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_map>

class Wad
{
private:
	Wad(const std::string& path);

	std::string filepath;
	char magic[5] = { 0 }; // The type
	uint32_t numDescriptors; // Number of descriptors = number of elements
	uint32_t descriptorOffset; 

	// Decriptor information represented as n-ary tree
	struct dNode {
		std::unordered_map<std::string, dNode*> children;
		std::vector<std::string> orderedContents;

		uint32_t element_offset = 0;
		uint32_t element_length = 0;
		std::string name;

		int type = 0; // 0 = directory, 1 = file
	};
	dNode* root;

	// Helper function to insert descriptors into the tree
	std::stack<dNode*> dStack;
	int mapCounter = 0;
	bool mapFlag = false;
	void insertDescriptor(dNode* node, std::ifstream& file);
	// Helper function to find node in tree
	dNode* findPath(const std::string path);
	bool isRightPath(const std::string path, std::stack<std::string>helpStack);

public:
	static Wad* loadWad(const std::string& path);
	std::string getMagic();
	bool isContent(const std::string& path);
	bool isDirectory(const std::string& path);
	int getSize(const std::string& path);
	int getContents(const std::string& path, char* buffer, int length, int offset = 0);
	int getDirectory(const std::string& path, std::vector<std::string>* driectory);
	void createDirectory(const std::string& path);
	void createFile(const std::string& path);
	int writeToFile(const std::string& path, const char* buffer, int length, int offset = 0);
};