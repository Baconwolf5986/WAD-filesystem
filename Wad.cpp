#include "Wad.h"

Wad::Wad(const std::string& path) {
	std::ifstream file;
	file.open(path);
	filepath = path;

	magic[4] = '\0';

	if (!file.is_open()) {
		std::cerr << "Error: Could not open file " << path << std::endl;
		return;
	}

	// Load Wad Header
	file.read(magic, 4);
	file.read((char*)&numDescriptors, 4);
	file.read((char*)&descriptorOffset, 4);

	// Init root directory as '/'
	root = new dNode();
	root->name = "/";

	// Start creating descriptors
	dNode* current = new dNode();
	dNode* temp = nullptr;
	dStack.push(root);
	file.seekg(descriptorOffset);
	char nameBuffer[9];
	nameBuffer[8] = '\0';
	while (!file.eof()) {
		if (!file.read((char*)&current->element_offset, 4)) {
			break;
		}

		if (!file.read((char*)&current->element_length, 4)) {
			break;
		}

		if (!file.read(nameBuffer, 8)) {
			break;
		}

		nameBuffer[8] = '\0';
		current->name = std::string(nameBuffer);

		insertDescriptor(current, file);

		current = new dNode();
	}

	delete current;
	file.close();
}

void Wad::insertDescriptor(dNode* node, std::ifstream& file) {
	std::string n;

	if (node->name.length() >= 6 && node->name.substr(node->name.length() - 6) == "_START") {
		if (!dStack.empty()) {
			node->name = node->name.substr(0, 2);
			dStack.top()->children[node->name] = node;
			dStack.top()->orderedContents.push_back(node->name);
		}
		dStack.push(node);
	}
	else if (node->name.length() >= 4 && node->name.substr(node->name.length() - 4) == "_END") {
		if (!dStack.empty()) {
			dStack.pop();
		}
	}
	else if (node->name.length() >= 4 && node->name[0] == 'E' && node->name[2] == 'M') {
		if (!dStack.empty()) {
			node->type = 2;
			dStack.top()->children[node->name] = node;
			dStack.top()->orderedContents.push_back(node->name);
		}
		dStack.push(node);
		mapCounter = 10;
		mapFlag = true;
	}
	else {
		if (!dStack.empty()) {
			node->type = 1;
			dStack.top()->children[node->name] = node;
			dStack.top()->orderedContents.push_back(node->name);
			mapCounter -= 1;
		}
	}

	if (mapCounter <= 0 && mapFlag == true) {
		mapCounter = 0;
		mapFlag = false;
		dStack.pop();
	}
}

Wad::dNode* Wad::findPath(const std::string path) {
	// Split path into segments

	if (path.find('/') != 0) {
		return nullptr;
	}

	std::stringstream ss(path);
	std::string segment;
	std::vector<std::string> pathSegments;
	while (std::getline(ss, segment, '/')) {
		if (!segment.empty()) {
			pathSegments.push_back(segment);
		}
	}

	if (pathSegments.size() == 0 && path != "/") {
		return nullptr;
	}

	// Traverse the tree
	dNode* current = root;
	bool found = false;
	for (int i = 0; i < pathSegments.size(); i++) {
		auto it = current->children.find(pathSegments[i]);
		if (it != current->children.end()) {
			current = current->children[pathSegments[i]];
		}
		else {
			return nullptr;
		}

	}

	return current;
}

Wad* Wad::loadWad(const std::string& path) {
	return new Wad(path);
}

std::string Wad::getMagic() {
	return (std::string)magic;
}

bool Wad::isContent(const std::string& path) {
	dNode* c = findPath(path);
	if (c != nullptr) {
		if (c->type == 1) {
			return true;
		}
	}

	return false;
}

bool Wad::isDirectory(const std::string& path) {
	dNode* c = findPath(path);
	if (c != nullptr) {
		if (c->type == 0 || c->type == 2) {
			return true;
		}
	}

	return false;
}

int Wad::getSize(const std::string& path) {
	dNode* c = findPath(path);
	if (isContent(path)) {
		return c->element_length;
	}

	return -1;
}

int Wad::getContents(const std::string& path, char* buffer, int length, int offset) {
	dNode* c = findPath(path);
	if (c == nullptr) {
		return -1;
	}
	if (c->type == 1) {
			std::ifstream file(filepath, std::ios::binary);
			file.seekg(c->element_offset + offset);

			// Adjust length based on element size
			length = std::min(length, (int)(c->element_length - offset));

			if (length <= 0) {
				return 0;
			}

			file.read(buffer, length);
			return length;
	}

	return -1;
}

int Wad::getDirectory(const std::string& path, std::vector<std::string>* directory) {
	dNode* c = findPath(path);

	if (c != nullptr && c->type != 1) {
		for (auto child : c->orderedContents) {
			directory->push_back(child);
		}

		return c->children.size();
	}

	return -1;
}

bool Wad::isRightPath(const std::string path, std::stack<std::string> helpStack) {
	std::stringstream ss(path);
	std::string segment;
	std::vector<std::string> pathSegments;
	while (std::getline(ss, segment, '/')) {
		if (!segment.empty()) {
			pathSegments.push_back(segment);
		}
	}

	if (helpStack.empty() && !pathSegments.empty()) {
		return false;
	}

	std::stack<std::string> compareStack;
	for (int i = 0; i < pathSegments.size(); i++) {
		compareStack.push(pathSegments[i]);
	}

	while (!compareStack.empty()) {
		if (helpStack.empty()) {
			return false;
		}
		if (helpStack.top() != compareStack.top()) {
			return false;
		}
		compareStack.pop();
		helpStack.pop();
	}

	return true;
}


void Wad::createDirectory(const std::string& path) {
	// Remove trailing slash if it exists:
	std::string fixedPath = path;
	if (fixedPath.back() == '/') {
		fixedPath.pop_back();
	}

	size_t n = fixedPath.find_last_of('/');
	std::string parentPath = fixedPath.substr(0, n + 1);
	std::string newDirName = fixedPath.substr(n + 1);

	if (newDirName.length() > 2) {
		std::cerr << "Error: Directory name too long" << std::endl;
		return;
	}

	dNode* parentNode = findPath(parentPath);
	if (parentNode == nullptr) {
		return;
	}

	dNode* startNode = new dNode();
	if (parentNode->children.find(newDirName) != parentNode->children.end()) {
		std::cerr << "Error: Directory already exists" << std::endl;
		return;
	}

	if (parentNode->name == newDirName) {
		std::cerr << "Error: Directory cannot share name" << std::endl;
		return;
	}

	if (parentNode->type == 2) {
		std::cerr << "Error: Directory cannot be created inside map marker" << std::endl;
		return;
	}

	startNode->element_length = 0;
	startNode->element_offset = 0;
	startNode->name = newDirName;
	startNode->type = 0;

	// Find the offset
	std::fstream file(filepath, std::ios::in | std::ios::out | std::ios::binary);
	int newItemOffset = 0;
	file.seekg(descriptorOffset);
	bool parentIsRoot = false;
	std::stack<std::string> helpStack;
	while (true) {
		char name[9];
		
		file.seekg(8, std::ios::cur);
		file.read(name, 8);
		name[8] = '\0';

		// If parent is root
		// Add directory to the end of the file
		if (parentPath == "/") {
			parentIsRoot = true;
			break;
		}

		if (std::string(name) == parentNode->name + "_END" && isRightPath(parentPath, helpStack)) {
			break;
		}

		std::string nameStr = std::string(name);

		if (nameStr.length() >= 6 && nameStr.substr(nameStr.length() - 6) == "_START") {
			nameStr = nameStr.substr(0, nameStr.length() - 6);
			helpStack.push(nameStr);
		}
		else if (nameStr.length() >= 4 && nameStr.substr(nameStr.length() - 4) == "_END") {
			helpStack.pop();
		}

		newItemOffset += 1;
	}

	// Save the previous data
	size_t movedStart = file.tellg();
	movedStart -= 16;
	file.seekg(0, std::ios::end);
	//size_t movedStart = descriptorOffset + (newItemOffset * 16);
	size_t movedEnd = file.tellg();
	size_t movedDataSize = movedEnd - movedStart;

	std::vector<char> movedData(movedDataSize);
	file.seekg(movedStart);
	file.read(movedData.data(), movedDataSize);

	if (parentIsRoot) {
		file.seekp(movedEnd);

		file.write("0000", 4);
		file.write("0000", 4);

		// Write "_START" padded to 8 characters
		std::string startName = (newDirName + "_START").substr(0, 8);
		startName.resize(8, '\0');
		file.write(startName.c_str(), 8);

		file.write("0000", 4);
		file.write("0000", 4);

		// Write "_END" padded to 8 characters
		std::string endName = (newDirName + "_END").substr(0, 8);
		endName.resize(8, '\0');
		file.write(endName.c_str(), 8);
	}
	else {
		// Increase the file and move the data up
		file.seekp(movedStart);
		file.write("0000", 4);
		file.write("0000", 4);
		// Write "_START" padded to 8 characters
		std::string startName = (newDirName + "_START").substr(0, 8);
		startName.resize(8, '\0');
		file.write(startName.c_str(), 8);

		file.write("0000", 4); // 4 bytes
		file.write("0000", 4); // 4 bytes

		// Write "_END" padded to 8 characters
		std::string endName = (newDirName + "_END").substr(0, 8);
		endName.resize(8, '\0');
		file.write(endName.c_str(), 8);
		file.write(movedData.data(), movedDataSize);
	}

	numDescriptors += 2;
	file.seekp(4, std::ios::beg);
	file.write((char*)&numDescriptors, 4);

	parentNode->children[startNode->name] = startNode;
	parentNode->orderedContents.push_back(startNode->name);

	file.close();
}

void Wad::createFile(const std::string& path) {
	// Remove trailing slash if it exists:
	std::string fixedPath = path;
	if (fixedPath.back() == '/') {
		fixedPath.pop_back();
	}

	size_t n = fixedPath.find_last_of('/');
	std::string parentPath = fixedPath.substr(0, n + 1);
	std::string newDirName = fixedPath.substr(n + 1);

	if (newDirName.length() > 8) {
		std::cerr << "Error: Directory name too long" << std::endl;
		return;
	}

	if (newDirName[0] == 'E' && newDirName[2] == 'M') {
		std::cerr << "Error: File name cannot start with 'E' and have 'M' as the third character" << std::endl;
		return;
	}

	dNode* parentNode = findPath(parentPath);
	if (parentNode == nullptr) {
		return;
	}

	if (parentNode->type == 2) {
		return;
	}

	if (parentNode->children.find(newDirName) != parentNode->children.end()) {
		std::cerr << "Error: File already exists" << std::endl;
		return;
	}

	dNode* startNode = new dNode();

	startNode->element_length = 0;
	startNode->element_offset = 0;
	startNode->name = newDirName;
	startNode->type = 1;

	parentNode->children[startNode->name] = startNode;
	parentNode->orderedContents.push_back(startNode->name);

	// Find descriptor
	std::fstream file(filepath, std::ios::in | std::ios::out | std::ios::binary);
	int newItemOffset = 0;
	file.seekg(descriptorOffset);
	bool parentIsRoot = false;
	std::stack<std::string> helpStack;
	while (true) {
		char name[9];
		file.seekg(8, std::ios::cur);
		file.read(name, 8);
		name[8] = '\0';

		if (parentPath == "/") {
			parentIsRoot = true;
			break;
		}

		if (std::string(name) == parentNode->name + "_END" && isRightPath(parentPath, helpStack)) {
			break;
		}

		std::string nameStr = std::string(name);

		if (nameStr.length() >= 6 && nameStr.substr(nameStr.length() - 6) == "_START") {
			nameStr = nameStr.substr(0, nameStr.length() - 6);
			helpStack.push(nameStr);
		}
		else if (nameStr.length() >= 4 && nameStr.substr(nameStr.length() - 4) == "_END") {
			helpStack.pop();
		}

		newItemOffset += 1;
	}

	// Save the previous data
	file.seekg(0, std::ios::end);
	size_t movedStart = descriptorOffset + (newItemOffset * 16);
	size_t movedEnd = file.tellg();
	size_t movedDataSize = movedEnd - movedStart;

	std::vector<char> movedData(movedDataSize);
	file.seekg(movedStart);
	file.read(movedData.data(), movedDataSize);

	if (parentIsRoot) {
		file.seekp(movedEnd);

		file.write("0000", 4);
		file.write("0000", 4);

		// Write "_START" padded to 8 characters
		std::string startName = (newDirName).substr(0, 8);
		startName.resize(8, '\0');
		file.write(startName.c_str(), 8);
	}
	else {
		// Increase the file and move the data up
		file.seekp(movedStart);
		file.write("0000", 4);
		file.write("0000", 4);
		// Write "_START" padded to 8 characters
		std::string startName = (newDirName).substr(0, 8);
		startName.resize(8, '\0');
		file.write(startName.c_str(), 8);

		file.write(movedData.data(), movedDataSize);
	}

	numDescriptors += 1;
	file.seekp(4, std::ios::beg);
	file.write((char*)&numDescriptors, 4);
	file.close();
}

int Wad::writeToFile(const std::string& path, const char* buffer, int length, int offset) {
	dNode* fileNode = findPath(path);

	std::string parentPath = path.substr(0, path.find_last_of('/'));

	if (fileNode == nullptr) {
		return -1;
	}

	if (fileNode->type != 1) {
		return -1;
	}

	if (fileNode->element_length != 0 && fileNode->element_offset != 0) {
		std::cerr << "Error: File already has data" << std::endl;
		return 0;
	}

	size_t lumpLocation = descriptorOffset;
	fileNode->element_length = length;
	fileNode->element_offset = lumpLocation;

	std::fstream file(filepath, std::ios::in | std::ios::out | std::ios::binary);

	file.seekg(descriptorOffset);
	std::stack<std::string> helpStack;
	while (!file.eof()) {
		char name[9];
		file.seekg(8, std::ios::cur);
		file.read(name, 8);
		name[8] = '\0';

		if ((std::string(name) == fileNode->name) && isRightPath(parentPath, helpStack)) {
			file.seekg(-16, std::ios::cur);
			size_t updateDescriptorLocation = file.tellg();
			file.seekp(updateDescriptorLocation, std::ios::beg);
			file.write((char*)&lumpLocation, 4);
			file.write((char*)&fileNode->element_length, 4);

			break;
		}
		
		std::string nameStr = std::string(name);
		if (nameStr.length() >= 6 && nameStr.substr(nameStr.length() - 6) == "_START") {
			nameStr = nameStr.substr(0, nameStr.length() - 6);
			helpStack.push(nameStr);
		}
		else if (nameStr.length() >= 4 && nameStr.substr(nameStr.length() - 4) == "_END") {
			helpStack.pop();
		}
	}

	std::vector<char> movedData;
	file.seekg(0, std::ios::end);
	size_t movedStart = lumpLocation;
	size_t movedEnd = file.tellg();
	size_t movedDataSize = movedEnd - movedStart;

	movedData.resize(movedDataSize);
	file.seekg(movedStart);
	file.read(movedData.data(), movedDataSize);

	file.seekp(lumpLocation + offset);
	file.write(buffer, length);
	file.write(movedData.data(), movedDataSize);

	descriptorOffset += length;
	file.seekp(8, std::ios::beg);
	file.write((char*)&descriptorOffset, 4);

	file.close();

	return length;
}