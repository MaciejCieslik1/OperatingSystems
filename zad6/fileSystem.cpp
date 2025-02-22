#include "fileSystem.h"


std::string fileSystemName = "filesystem";
SuperBlock* superBlockPtr = new SuperBlock();

//-----------------------------------------------------------------
// Key functions

int createFileSystem() {
    std::fstream filePtr;
    SIZE systemSize;
    SIZE totalBlocksNumber;

    filePtr.open(fileSystemName, std::ios::out | std::ios::binary);
    if (!filePtr) {
        std::cerr << "Failed to create filesystem" << std::endl;
        return -1;
    }

    totalBlocksNumber = MAX_FILES_NUMBER;
    systemSize = sizeof(SuperBlock) + BLOCKS_NUMBER * (INODE_SIZE + BLOCK_SIZE);

    if (!superBlockPtr) {
        std::cerr << "Failed to create super block" << std::endl;
        filePtr.close();
        return -1;
    }

    superBlockPtr->systemSize = systemSize;
    superBlockPtr->totalBlocksNumber = totalBlocksNumber;
    superBlockPtr->firstINodeIndex = 0;
    superBlockPtr->usedUserSpace = 0; 
    superBlockPtr->freeUserSpace = totalBlocksNumber * BLOCK_SIZE_DATA;

    if (!filePtr.write(reinterpret_cast<const char*>(superBlockPtr), sizeof(SuperBlock))) {
        std::cerr << "Failed to save super block" << std::endl;
        filePtr.close();
        delete superBlockPtr;
        return -1;
    }

    filePtr.close();
    std::cout << "File system successfully created" << std::endl;
    return 1;
}


int createFile(const SIZE& fileSize, const std::string& fileName)
{
    if (fileSize > superBlockPtr->freeUserSpace) {
        std::cerr << "Failed to create the file. The file size is too big" << std::endl;
        return -1;
    }

    if (fileName.length() > MAX_NAME_LENGTH) {
        std::cerr << "Failed to create the file. The file name is too long" << std::endl;
        return -1;
    }


    if (!isFileNameValid(fileName)) {
        std::cerr << "Failed to create file. File with that name already exists" << std::endl;
        return -1;
    }

    std::vector<bool> blocksMap = prepareBlocksMap();

    int requiredBlocksNumber = countRequiredBlocksNumber(fileSize);
    int freeBlocksNumber = countFreeBlocksNumber(blocksMap);

    if (requiredBlocksNumber > freeBlocksNumber) {
        std::cerr << "Failed to create file. Not enough free memory blocks" << std::endl;
        return -1;
    }

    std::vector<char> data(fileSize, 0xFF); 
    bool isCreated = saveFile(blocksMap, requiredBlocksNumber, data, fileSize, fileName);

    if (!isCreated) {
        std::cerr << "Failed to create file" << std::endl;
        return -1;
    }

    std::cout << "File successfully created" << std::endl;
    return 1;
}


int downloadFileFromPhysicalDisk(const std::string& fileName) {
    std::fstream physicalFilePtr(fileSystemName, std::ios::in | std::ios::binary);
    if (!physicalFilePtr) {
        std::cerr << "Failed to open file on physical disk. File does not exist" << std::endl;
        physicalFilePtr.close();
        return -1;
    }

    physicalFilePtr.seekg(0, std::ios::end);

    int fileSize = physicalFilePtr.tellg();
    if (fileSize > superBlockPtr->freeUserSpace) {
        std::cerr << "Failed to download file from physical disk. File is too big" << std::endl;
        physicalFilePtr.close();
        return -1;
    }

    physicalFilePtr.seekg(0);

    std::vector<char> data(fileSize);
    physicalFilePtr.read(data.data(), fileSize);
    physicalFilePtr.close();

    std::fstream filePtr(fileSystemName, std::ios::in | std::ios::out | std::ios::binary);
    if (!filePtr) {
        std::cerr << "Failed to run file system" << std::endl;
        return -1;
    }

    if (!isFileNameValid(fileName)) {
        std::cerr << "Failed to create file. File with that name already exists" << std::endl;
        return -1;
    }

    filePtr.seekg(0);

    std::vector<bool> blocksMap = prepareBlocksMap();
    filePtr.close();

    int requiredBlocksNumber = countRequiredBlocksNumber(fileSize);
    int freeBlocksNumber = countFreeBlocksNumber(blocksMap);

    if (requiredBlocksNumber > freeBlocksNumber) {
        std::cerr << "Failed create file. Not enough free memory blocks" << std::endl;
        return -1;
    }

    bool isCreated = saveFile(blocksMap, requiredBlocksNumber, data, fileSize, fileName);

    if (!isCreated) {
        std::cerr << "Failed to create file" << std::endl;
        return -1;
    }
    std::cout << "File successfully downloaded" << std::endl;
    return 1;
}


int uploadFileToPhysicalDisk(const std::string& fileNameStr) {
    const char* fileName = fileNameStr.c_str();
    if (access(fileName, F_OK) == 0) {
        std::cerr << "Failed to upload file to physical disk. File with this filename already exists.";
        return -1;
    }

    std::fstream filePtr(fileSystemName, std::ios::in | std::ios::binary);
    if (!filePtr) {
        std::cerr << "Failed to run file system" << std::endl;
        return -1;
    }

    std::vector<char> data = getDataFromFile(filePtr, fileName);
    filePtr.close();

    std::fstream PhysicalfilePtr(fileName, std::ios::out | std::ios::binary);
    if (!PhysicalfilePtr) {
        std::cerr << "Failed to open the physical file" << std::endl;
        return -1;
    }

    if (!filePtr.write(data.data(), data.size())) {
        std::cerr << "Failed to save file on physical disk" << std::endl;
        filePtr.close();
        return -1;
    }
    std::cout << "File successfully uploaded" << std::endl;
    return 1;
}


int listFiles() {
    std::fstream filePtr(fileSystemName, std::ios::in | std::ios::binary);
    if (!filePtr) {
        std::cerr << "Failed to open file system" << std::endl;
        return -1;
    }

    filePtr.seekg(INODES_OFFSET);

    std::vector<char> iNodeVec(INODE_SIZE);
    char fileNameTable[MAX_NAME_LENGTH];
    for (int iNodeIndex = 0; iNodeIndex < MAX_FILES_NUMBER; iNodeIndex++) {
        filePtr.read(iNodeVec.data(), INODE_SIZE);
        for (int i = 0; i < MAX_NAME_LENGTH; i++) {
            fileNameTable[i] = iNodeVec[i];
        }
        if (!std::all_of(std::begin(fileNameTable), std::end(fileNameTable), [](char c) { return c == 0; })) {
            std::string fileName(fileNameTable);
            std::cout << fileName << std::endl; 
        }
    }
    return 1;
}


int deleteFile(const std::string& fileName) {
    std::fstream filePtr;
    if (!filePtr) {
        std::cerr << "Failed to open file system" << std::endl;
        return -1;
    }

    filePtr.seekg(INODES_OFFSET);

    std::vector<char> iNodeVec(INODE_SIZE);
    char fileNameTable[MAX_NAME_LENGTH];
    int choosenINodeIndex;
    for (int iNodeIndex = 0; iNodeIndex < MAX_FILES_NUMBER; iNodeIndex++) {
        filePtr.read(iNodeVec.data(), INODE_SIZE);
        for (int i = 0; i < MAX_NAME_LENGTH; i++) {
            fileNameTable[i] = iNodeVec[i];
        }
        if (fileNameTable == fileName) {
            choosenINodeIndex = iNodeIndex;
            break;
        }
    }

    char byte1 = static_cast<char>(iNodeVec[iNodeVec.size() - 4]);
    char byte2 = static_cast<char>(iNodeVec[iNodeVec.size() - 3]);
    char byte3 = static_cast<char>(iNodeVec[iNodeVec.size() - 2]);
    char byte4 = static_cast<char>(iNodeVec[iNodeVec.size() - 1]);

    SIZE nextDataBlockIndex = (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;

    const char emptyFileName[MAX_NAME_LENGTH] = {};
    if(saveINode(emptyFileName, 0, 0, choosenINodeIndex) == -1){
        std::cerr << "Failed to delete file. Cannot save empty iNode" << std::endl;
        return -1;
    }

    std::vector<char> nextDataBlockVecIndex(sizeof(SIZE));
    while (nextDataBlockIndex != -1) {
        filePtr.seekg(BLOCKS_OFFSET + BLOCK_SIZE * nextDataBlockIndex + BLOCK_SIZE_DATA);
        filePtr.read(nextDataBlockVecIndex.data(), sizeof(SIZE));

        byte1 = static_cast<char>(nextDataBlockVecIndex[nextDataBlockVecIndex.size() - 4]);
        byte2 = static_cast<char>(nextDataBlockVecIndex[nextDataBlockVecIndex.size() - 3]);
        byte3 = static_cast<char>(nextDataBlockVecIndex[nextDataBlockVecIndex.size() - 2]);
        byte4 = static_cast<char>(nextDataBlockVecIndex[nextDataBlockVecIndex.size() - 1]);

        SIZE nextDataBlockIndex = (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;

        filePtr.seekg(BLOCKS_OFFSET + BLOCK_SIZE * nextDataBlockIndex);


        Block* blockPtr = new Block();
        std::memset(blockPtr->data, 0, BLOCK_SIZE);
        
        if (!filePtr.write(reinterpret_cast<const char*>(blockPtr), sizeof(Block))) {
            std::cerr << "Failed to save data" << std::endl;
            delete blockPtr;
            return -1;
        }
        delete blockPtr;
    }
    std::cout << "File successfully deleted" << std::endl;
    return 1;
} 


int deleteFileSystem() {
    if (!std::remove(fileSystemName.c_str()) != 0) {
        std::cerr << "Failed to delete file system" << std::endl;
        return -1;
    }
    std::cout << "File system successfully deleted" << std::endl;
    return 1;
}


int displayInfo() {
    std::cout << "\nMain info about file system" << std::endl;
    std::cout << "Max files naumber: " << MAX_FILES_NUMBER <<  std::endl;
    std::cout << "Blocks number: " << BLOCKS_NUMBER <<  std::endl;
    std::cout << "Max name lenght: " << MAX_NAME_LENGTH <<  std::endl;

    std::cout << "\nSuper block parameters" << std::endl;
    std::cout << "System size: " << superBlockPtr->systemSize <<  std::endl;
    std::cout << "Total blocks number: " << superBlockPtr->totalBlocksNumber <<  std::endl;
    std::cout << "First INode index: " << superBlockPtr->firstINodeIndex <<  std::endl;
    std::cout << "Used user space: " << superBlockPtr->usedUserSpace <<  std::endl;
    std::cout << "Free user space: " << superBlockPtr->freeUserSpace <<  std::endl;

    if(printINodesState() == -1) {
        std::cerr << "Failed to display info: Failed to read Inodes" << std::endl;
        return -1;
    }
    if(printDataBlocksState() == -1) {
        std::cerr << "Failed to display info: Failed to read data blocks" << std::endl;
        return -1;
    }
    return 1;
}


int main() {
    std::string command;
    bool quitFlag = false;
    while (!quitFlag) {
        std::cout << "\n\n##############FILE SYSTEM##############" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "1. Create file system" << std::endl;
        std::cout << "2. Create file" << std::endl;
        std::cout << "3. Download file from physical disk" << std::endl;
        std::cout << "4. Upload file to physical disk" << std::endl;
        std::cout << "5. List files" << std::endl;
        std::cout << "6. Delete file" << std::endl;
        std::cout << "7. Delete file system" << std::endl;
        std::cout << "8. Display system info" << std::endl;
        std::cout << "9. Quit" << std::endl;
        std::cout << "#######################################" << std::endl;
        std::cout << "Enter command: ";
        std::cin >> command;

        try {
            int commandInt = std::stoi(command);
            std::string fileName;
            SIZE fileSize = 0;
            switch (commandInt) {
                case 1:
                    createFileSystem();
                    break;
                case 2:
                    fileSize = getFileSizeFromUser();
                    fileName = getFileNameFromUser();
                    createFile(fileSize, fileName);
                    break;
                case 3:
                    fileName = getFileNameFromUser();
                    downloadFileFromPhysicalDisk(fileName);
                    break;
                case 4:
                    fileName = getFileNameFromUser();
                    uploadFileToPhysicalDisk(fileName);
                    break;
                case 5:
                    listFiles();
                    break;
                case 6:
                    fileName = getFileNameFromUser();
                    deleteFile(fileName);
                    break;
                case 7:
                    deleteFileSystem();
                    break;
                case 8:
                    displayInfo();
                    break;
                case 9:
                    quitFlag = true;
                    break;
                default:
                    std::cout << "Invalid choice" << std::endl;
            }
        } 
        catch (const std::invalid_argument& e) {
            std::cout << "Invalid choice" << std::endl;
        }
    }
    delete superBlockPtr;
    return 0;
}


//-----------------------------------------------------------------
// Helper functions

int countRequiredBlocksNumber(const SIZE& fileSize) {
    int requiredBlocksNumber = 0;
    if (fileSize % BLOCK_SIZE_DATA == 0) {
        requiredBlocksNumber = fileSize / BLOCK_SIZE_DATA;
    } 
    else {
        requiredBlocksNumber = fileSize / BLOCK_SIZE_DATA + 1;
    }
    return requiredBlocksNumber;
}


int countFreeBlocksNumber(const std::vector<bool>& blocksMap) {
    int freeBlocksNumber = 0;
    for (bool isBlockFilled: blocksMap) {
        if (!isBlockFilled) freeBlocksNumber ++;
    }
    return freeBlocksNumber;
}


std::vector<bool> prepareBlocksMap() {
    std::fstream filePtr;
    filePtr.open(fileSystemName, std::ios::in | std::ios::binary);
    std::vector<bool> blocksMap(BLOCKS_NUMBER, false);
    std::vector<char> blockVec(BLOCK_SIZE);
    bool isZeroFilled;
    filePtr.seekg(BLOCKS_OFFSET);

    for (int blockIndex = 0; blockIndex < BLOCKS_NUMBER; blockIndex++) {
        filePtr.read(blockVec.data(), BLOCK_SIZE);
        isZeroFilled = std::all_of(blockVec.begin(), blockVec.end(), [](char c) { return c == 0; });
        if (!isZeroFilled) {
            blocksMap[blockIndex] = true;
        }
    }
    filePtr.close();
    return blocksMap;
}

std::vector<int> prepareEmptyBlocksIndexes(const std::vector<bool>& blocksMap) {
    std::vector<int> emptyBlocksIndexes;
    for (int blockIndex = 0; blockIndex < MAX_FILES_NUMBER; blockIndex++) {
        if (blocksMap[blockIndex] == false) emptyBlocksIndexes.push_back(blockIndex);
    }
    return emptyBlocksIndexes;
}


bool saveFile(const std::vector<bool>& blocksMap, const int& requiredBlocksNumber, const std::vector<char>& data, 
                const SIZE& fileSize, const std::string& fileName) {
    std::fstream filePtr;
    filePtr.open(fileSystemName, std::ios::in | std::ios::out | std::ios::binary);
    if (!filePtr) {
        std::cerr << "Failed to run file system" << std::endl;
        return false;
    }

    std::vector<int> emptyBlocksIndexes = prepareEmptyBlocksIndexes(blocksMap);
    int firstFreeINodeIndex = getFirstFreeINodeIndex();
    filePtr.close();
    if (firstFreeINodeIndex == -1) {
        std::cerr << "Failed to read first INode" << std::endl;
        return false;
    }
    if (saveINode(fileName, fileSize, emptyBlocksIndexes[0], firstFreeINodeIndex) == -1) {
        std::cerr << "Failed to save INode" << std::endl;
        filePtr.close();
        return false;
    }

    if (saveDataBlocks(emptyBlocksIndexes, requiredBlocksNumber, fileSize, data) == -1) {
        std::cerr << "Failed to save data block" << std::endl;
        filePtr.close();
        return false;
    }
    
    if (saveSuperBlock(requiredBlocksNumber) == -1) {
        std::cerr << "Failed to save super block" << std::endl;
        filePtr.close();
        return false;
    }

    filePtr.close();
    return true;
}


int getFirstFreeINodeIndex() {
    std::fstream filePtr;
    filePtr.open(fileSystemName, std::ios::in | std::ios::out | std::ios::binary);
    int firstFreeINodeIndex;
    std::vector<char> iNodeVec(INODE_SIZE);

    filePtr.seekg(INODES_OFFSET);

    bool isZeroFilled;
    for (int iNodeIndex = 0; iNodeIndex < MAX_FILES_NUMBER; iNodeIndex++) {
        filePtr.read(iNodeVec.data(), INODE_SIZE);
        isZeroFilled = std::all_of(iNodeVec.begin(), iNodeVec.end(), [](char c) { return c == 0; });
        if (isZeroFilled) {
            firstFreeINodeIndex = iNodeIndex;
            break;
        }
    }
    filePtr.close();
    return firstFreeINodeIndex;
}


int saveINode(const std::string& fileNameStr, const SIZE& fileSize, const int& firstBlockIndex, const int& firstFreeINodeIndex) {
    std::fstream filePtr;
    filePtr.open(fileSystemName, std::ios::out | std::ios::binary);
    filePtr.seekp(INODES_OFFSET + INODE_SIZE * firstFreeINodeIndex);

    INode* iNodePtr = new INode();
    const char* fileName = fileNameStr.c_str();
    strncpy(iNodePtr->fileName, fileName, MAX_NAME_LENGTH - 1);
    iNodePtr->fileSize = fileSize;
    iNodePtr->indexOfFirstBlock = firstBlockIndex;

    if (!filePtr.write(reinterpret_cast<const char*>(iNodePtr), sizeof(INode))) {
        delete iNodePtr;
        return -1;
    }
    delete iNodePtr;
    return 1;
}


int saveDataBlocks(const std::vector<int>& emptyBlocksIndexes, 
                    const int& requiredBlocksNumber, const SIZE& fileSize, const std::vector<char>& data) {
    std::fstream filePtr;
    filePtr.open(fileSystemName, std::ios::out | std::ios::binary);
    int currentFilledBlockNumber = 0;
    int beggingDataBlockIndex;
    int endingDataBlockIndex;
    int blockIndex;
    for (int j = 0; j < emptyBlocksIndexes.size(); j++) {
        blockIndex = emptyBlocksIndexes[j];
        if (currentFilledBlockNumber >=  requiredBlocksNumber) {
            break;
        }

        filePtr.seekp(BLOCKS_OFFSET + BLOCK_SIZE * blockIndex);
        
        beggingDataBlockIndex = currentFilledBlockNumber * BLOCK_SIZE_DATA;
        if ((currentFilledBlockNumber + 1) * BLOCK_SIZE_DATA > fileSize) endingDataBlockIndex = fileSize;
        else endingDataBlockIndex = (currentFilledBlockNumber + 1) * BLOCK_SIZE_DATA;

        char blockData[BLOCK_SIZE];
        int counter = 0;
        for (int i = beggingDataBlockIndex; i < endingDataBlockIndex; i++) {
            blockData[counter] = data[i];
            counter ++;
        }
        
        Block* blockPtr = new Block();
        std::memcpy(blockPtr->data, blockData, BLOCK_SIZE);
        if (requiredBlocksNumber - currentFilledBlockNumber > 1 ) blockPtr->nextBlockIndex = emptyBlocksIndexes[j + 1];
        else blockPtr->nextBlockIndex = -1; // -1 means that this is the last block of the file
        
        if (!filePtr.write(reinterpret_cast<const char*>(blockPtr), sizeof(Block))) {
            std::cerr << "Failed to save data" << std::endl;
            delete blockPtr;
            return -1;
        }

        delete blockPtr;
        currentFilledBlockNumber ++;
    }
    return 1;
}


int saveSuperBlock(const int& requiredBlocksNumber) {
    std::fstream filePtr;
    filePtr.open(fileSystemName, std::ios::out | std::ios::binary);

    superBlockPtr->usedUserSpace = superBlockPtr->usedUserSpace + requiredBlocksNumber * BLOCK_SIZE_DATA; 
    superBlockPtr->freeUserSpace = MAX_FILES_NUMBER * BLOCK_SIZE - superBlockPtr->usedUserSpace;
    if (!filePtr.write(reinterpret_cast<const char*>(superBlockPtr), sizeof(SuperBlock))) {
        std::cerr << "Failed to save super block" << std::endl;
        filePtr.close();
        return -1;
    }
    filePtr.close();
    return 1;
}


bool isFileNameValid(const std::string& fileName) {
    std::fstream filePtr;
    filePtr.open(fileSystemName, std::ios::in | std::ios::out | std::ios::binary);
    filePtr.seekg(INODES_OFFSET);

    std::vector<char> iNodeVec(INODE_SIZE);
    char fileNameTable[MAX_NAME_LENGTH];
    for (int iNodeIndex = 0; iNodeIndex < MAX_FILES_NUMBER; iNodeIndex++) {
        filePtr.read(iNodeVec.data(), INODE_SIZE);
        for (int i = 0; i < MAX_NAME_LENGTH; i++) {
            fileNameTable[i] = iNodeVec[i];
        }
        if (fileNameTable == fileName) {
            filePtr.close();
            return false;
        }
    }
    filePtr.close();
    return true;
}


std::vector<char> getDataFromFile(std::fstream& filePtr, const std::string& fileName) {
    filePtr.seekg(INODES_OFFSET);

    std::vector<char> iNodeVec(INODE_SIZE);
    char fileNameTable[MAX_NAME_LENGTH];
    for (int iNodeIndex = 0; iNodeIndex < MAX_FILES_NUMBER; iNodeIndex++) {
        filePtr.read(iNodeVec.data(), INODE_SIZE);
        for (int i = 0; i < MAX_NAME_LENGTH; i++) {
            fileNameTable[i] = iNodeVec[i];
        }
        if (fileNameTable == fileName) {
            break;
        }
    }

    char byte1 = static_cast<char>(iNodeVec[iNodeVec.size() - 4]);
    char byte2 = static_cast<char>(iNodeVec[iNodeVec.size() - 3]);
    char byte3 = static_cast<char>(iNodeVec[iNodeVec.size() - 2]);
    char byte4 = static_cast<char>(iNodeVec[iNodeVec.size() - 1]);

    SIZE nextDataBlockIndex = (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;

    std::vector<char> data;
    std::vector<char> dataBlockVec;
    std::vector<char> nextDataBlockVecIndex;
    while (nextDataBlockIndex != -1) {
        filePtr.seekg(BLOCKS_OFFSET + BLOCK_SIZE * nextDataBlockIndex);
        filePtr.read(dataBlockVec.data(), BLOCK_SIZE_DATA);
        filePtr.read(nextDataBlockVecIndex.data(), sizeof(SIZE));

        byte1 = static_cast<char>(nextDataBlockVecIndex[nextDataBlockVecIndex.size() - 4]);
        byte2 = static_cast<char>(nextDataBlockVecIndex[nextDataBlockVecIndex.size() - 3]);
        byte3 = static_cast<char>(nextDataBlockVecIndex[nextDataBlockVecIndex.size() - 2]);
        byte4 = static_cast<char>(nextDataBlockVecIndex[nextDataBlockVecIndex.size() - 1]);

        SIZE nextDataBlockIndex = (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;

        data.insert(data.end(), dataBlockVec.begin(), dataBlockVec.end());
    }
    return data;
}


int printINodesState() {
    std::fstream filePtr;
    filePtr.open(fileSystemName, std::ios::in | std::ios::binary);
    if (!filePtr.is_open()) {
        std::cerr << "Failed to run file system" << std::endl;
        return -1;
    }
    filePtr.seekg(INODES_OFFSET);
    std::vector<char> iNodeVec(INODE_SIZE);
    SIZE fileSize = 0;
    SIZE firstDataBlockIndex = 0;
    for (int iNodeIndex = 0; iNodeIndex < MAX_FILES_NUMBER; iNodeIndex++) {
        filePtr.read(iNodeVec.data(), INODE_SIZE);

        std::vector<char> fileNameVec(iNodeVec.begin(), iNodeVec.begin() + MAX_NAME_LENGTH);
        std::vector<char> fileSizeVec(iNodeVec.begin() + MAX_NAME_LENGTH, iNodeVec.begin() + MAX_NAME_LENGTH + sizeof(SIZE));
        std::vector<char> firstDataBlockIndexVec(iNodeVec.begin() + MAX_NAME_LENGTH + sizeof(SIZE), iNodeVec.end());

        std::string fileName(fileNameVec.begin(), fileNameVec.end());
        memcpy(&fileSize, fileSizeVec.data(), sizeof(SIZE));
        memcpy(&firstDataBlockIndex, firstDataBlockIndexVec.data(), sizeof(SIZE));

        std::cout << "\nINode block index: " << iNodeIndex << ", file name: " << fileName << ", file size: " << fileSize << ", first data block index: " << firstDataBlockIndex << std::endl;
    }
    filePtr.close();
    return 1;
}


int printDataBlocksState() {
    std::fstream filePtr;
    filePtr.open(fileSystemName, std::ios::in | std::ios::binary);
    if (!filePtr.is_open()) {
        std::cerr << "Failed to run file system" << std::endl;
        return -1;
    }
    filePtr.seekg(BLOCKS_OFFSET);
    std::vector<char> dataBlockVec(BLOCK_SIZE);
    SIZE dataDigit = 0;
    SIZE nextBlockIndex = 0;
    for (int dataBlockIndex = 0; dataBlockIndex < MAX_FILES_NUMBER; dataBlockIndex++) {
        filePtr.read(dataBlockVec.data(), BLOCK_SIZE);
        std::vector<char> dataVec(dataBlockVec.begin(), dataBlockVec.begin() + BLOCK_SIZE_DATA);
        std::vector<char> nextBlockIndexVec(dataBlockVec.begin() + BLOCK_SIZE_DATA, dataBlockVec.end());

        for (char byte: dataVec) {
            if (byte != 0) {
                dataDigit = 1;
                break;
            }
        }

        memcpy(&nextBlockIndex, nextBlockIndexVec.data(), sizeof(SIZE));

        std::cout << "\nData block index: " << dataBlockIndex << ", data: " << dataDigit << ", next block index: " << nextBlockIndex << std::endl;

    }
    filePtr.close();
    return 1;
}


SIZE getFileSizeFromUser() {
    std::string fileSizeStr;
    SIZE fileSize;
    bool isCorrectSize = false;
    while (!isCorrectSize) {
        std::cout << "\nEnter file size: ";
        std::cin >> fileSizeStr;
        try {
            fileSize = std::stoi(fileSizeStr);
            if (fileSize > 0) {
                isCorrectSize = true;
            }
        }
        catch (const std::invalid_argument& e) {
            std::cout << "Invalid file size" << std::endl;
        }
    }
    return fileSize;
}


std::string getFileNameFromUser() {
    std::string fileName;
    std::cout << "\nEnter file name: ";
    std::cin >> fileName;
    return fileName;
}