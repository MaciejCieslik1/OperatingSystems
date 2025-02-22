#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <glob.h>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unistd.h> 
#include <filesystem>

#define MAX_FILES_NUMBER 64
#define BLOCKS_NUMBER 64
#define MAX_NAME_LENGTH 32
#define BLOCK_SIZE_DATA 4096
#define INODES_OFFSET sizeof(SuperBlock)
#define INODE_SIZE sizeof(INode)
#define BLOCK_SIZE BLOCK_SIZE_DATA + 4
#define BLOCKS_OFFSET INODES_OFFSET + MAX_FILES_NUMBER * sizeof(INode)

using SIZE = uint32_t; // 32 bits == 4 baits

// 5 * 4(SIZE) = 20
struct SuperBlock {
    SIZE systemSize; // size of the file system
    SIZE totalBlocksNumber;
    SIZE firstINodeIndex;
    SIZE usedUserSpace;
    SIZE freeUserSpace;
};

// 32(MAX_NAME_LENGTH) + 4 + 4 = 40
struct INode {
    char fileName[MAX_NAME_LENGTH];
    SIZE fileSize;
    SIZE indexOfFirstBlock;
};


// 4096(BLOCK_SIZE_DATA) + 4 = 4100
struct Block {
    char data[BLOCK_SIZE];
    int nextBlockIndex;
};


int createFileSystem();

int createFile(const SIZE& fileSize, const std::string& fileName);

int downloadFileFromPhysicalDisk(const std::string& fileName);

int uploadFileToPhysicalDisk(const std::string& fileName);

int listFiles();

int deleteFile(const std::string& fileName);

int deleteFileSystem();

int displayInfo();

// Helper functions

int countRequiredBlocksNumber(const SIZE& fileSize);

int countFreeBlocksNumber(const std::vector<bool>& blocksMap);

std::vector<bool> prepareBlocksMap();

std::vector<int> prepareEmptyBlocksIndexes(const std::vector<bool>& blocksMap);

bool saveFile(const std::vector<bool>& blocksMap, const int& requiredBlocksNumber, const std::vector<char>& data, 
                const SIZE& fileSize, const std::string& fileName);

int getFirstFreeINodeIndex();

int saveINode(const std::string& fileNameStr, const SIZE& fileSize, const int& firstBlockIndex, const int& firstFreeINodeIndex);

int saveDataBlocks(const std::vector<int>& emptyBlocksIndexes, 
                    const int& requiredBlocksNumber, const SIZE& fileSize, const std::vector<char>& data);

int saveSuperBlock(const int& requiredBlocksNumber);

bool isFileNameValid(const std::string& fileName);

std::vector<char> getDataFromFile(std::fstream& filePtr, const std::string& fileNameStr);

int printINodesState();

int printDataBlocksState();

SIZE getFileSizeFromUser();

std::string getFileNameFromUser();

#endif
