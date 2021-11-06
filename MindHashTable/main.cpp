#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <list>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

struct HashTableNode
{
	int deep = 0;
	uint64_t key = 0;
	std::string value = "";
	std::vector<HashTableNode*> nextTable;
};

class HashTable
{
public:
	size_t sizeTable = 0;
	std::vector<HashTableNode*> table;

	HashTable() {}
	~HashTable() {}

	void Reserve(size_t tablesize)
	{
		sizeTable = tablesize;
		table.resize(tablesize);
	}

	std::string Lookup(const uint64_t key);
	int Insert(const uint64_t key, const std::string val);
	void LoadFromFile(const char* filePath);
};

std::string HashTable::Lookup(const uint64_t key)
{
	int deep = 0;
	uint64_t pos = key % sizeTable;
	HashTableNode* temp = table[pos];
	while (temp)
	{
		if (temp->key == key)
		{
			return temp->value;
		}
		deep++;
		pos = (key + deep) % sizeTable;
		temp = temp->nextTable[pos];
	}
	return std::string();
}

int HashTable::Insert(const uint64_t key, const std::string val)
{
	if (HashTable::Lookup(key).size() == 0)
	{
		uint64_t pos = key % sizeTable;
		//printf("%016" PRIX64 " %s\n", key, val.c_str());
		if (table[pos])
		{
			int deep = 0;
			std::vector<HashTableNode*>* tempSave = nullptr;
			HashTableNode* temp = table[pos];
			while (temp)
			{
				if (temp->key == key)
				{
					temp->value = val;
					return 0;
				}
				deep++;
				tempSave = &temp->nextTable;
				pos = (key + deep) % sizeTable;
				temp = temp->nextTable[pos];
				//printf(">Pos %" PRIu64 ", Deep %d ", pos, deep);
			}
			HashTableNode* newNode = new HashTableNode;
			newNode->deep = deep;
			newNode->key = key;
			newNode->value = val;
			newNode->nextTable.resize(sizeTable);
			(*tempSave)[pos] = newNode;
			//printf("\n");
		}
		else
		{
			//printf(">Adding a root tree, Pos %" PRIu64 "\n", pos);
			HashTableNode* newNode = new HashTableNode;
			newNode->key = key;
			newNode->value = val;
			newNode->nextTable.resize(sizeTable);
			table[pos] = newNode;
		}
		return 1;
	}
	return 0;
}

void HashTable::LoadFromFile(const char* filePath)
{
	std::ifstream ifs(filePath);
	if (!ifs.is_open())
	{
		char errMsg[255] = { "\0" };
		strerror_s(errMsg, 255, errno);
		printf("ERROR: Cannot read file %s %s\n", filePath, errMsg);
		return;
	}

	size_t lines = 0;
	std::string line = "";

	while (std::getline(ifs, line))
	{
		const size_t hashEnd = line.find(" ");

		uint64_t key = 0;
		if (hashEnd == 8)
			key = strtoul(line.c_str(), nullptr, 16);
		else if (hashEnd == 16)
			key = strtoull(line.c_str(), nullptr, 16);
		else
			continue;

		const std::string value = line.data() + hashEnd + 1;
		lines += HashTable::Insert(key, value);

		if (lines >= 10)
			break;
	}

	ifs.close();

	//printf("File: %s loaded: %zd lines\n", filePath, lines);
}

void PrintIndention(int indention)
{
	printf("%*s", indention, "    ");
}

void PrintTableRecursion(std::vector<HashTableNode*>& hashTableVector, size_t sizeTable, int deep)
{
	for (size_t i = 0; i < sizeTable; i++)
	{
		PrintIndention(deep * 4);
		printf("\"%zd\":", i);
		HashTableNode* temp = hashTableVector[i];
		if (temp)
		{
			int deepFix = (deep + 1) * 4;
			printf(" {\n");

				PrintIndention(deepFix);
				printf("\"deep\": %d,\n", temp->deep);
				PrintIndention(deepFix);
				printf("\"key\": 0x%016" PRIX64 ",\n", temp->key);
				PrintIndention(deepFix);
				printf("\"value\": \"%s\",\n", temp->value.c_str());

				PrintIndention(deepFix);
				printf("\"Table\":\n");

				PrintIndention(deepFix);
				printf("{\n");
					PrintTableRecursion(temp->nextTable, sizeTable, deep + 2);
				PrintIndention(deepFix);
				printf("}\n");

			PrintIndention(deep * 4);
			printf("}");
			if (i < sizeTable - 1)
				printf(",");
			printf("\n");
		}
		else {
			printf(" {}");
			if (i < sizeTable - 1)
				printf(",");
			printf("\n");
		}
	}
}

void PrintTable(HashTable& hashT)
{
	printf("{\n");
	PrintIndention(4);
		printf("\"Table\":\n");
		PrintIndention(4);
		printf("{\n");
			PrintTableRecursion(hashT.table, hashT.sizeTable, 2);
		PrintIndention(4);
		printf("}\n");
	printf("}");
}

 uint32_t FNV1Hash(std::string string)
{
	uint32_t Hash = 0x811c9dc5;
	for (size_t i = 0; i < string.size(); i++)
		Hash = (Hash ^ tolower(string[i])) * 0x01000193;
	return Hash;
}

int main()
{
	HashTable hashT;
	hashT.Reserve(3);

	hashT.LoadFromFile("test.txt");

	//std::string test1 = "test1";
	//std::string test2 = "test2";
	//std::string test3 = "test3";
	//std::string test4 = "test4";
	//hashT.Insert(FNV1Hash(test1), test1);
	//hashT.Insert(FNV1Hash(test2), test2);
	//hashT.Insert(FNV1Hash(test3), test3);
	//hashT.Insert(FNV1Hash(test4), test4);

	PrintTable(hashT);

	printf("%s\n", hashT.Lookup(0xea452f2f).c_str());

	return 0;
}