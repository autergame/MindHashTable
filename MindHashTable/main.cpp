#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <list>
#include <vector>
#include <string>
#include <iomanip>
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
	int Insert(const uint64_t key, const std::string& val);
	void LoadFromFile(const char* filePath);
};

uint64_t murmurmix(uint64_t value)
{
	value ^= value >> 32ULL;
	value *= 0xff51afd7ed558ccdULL;
	value ^= value >> 32ULL;
	value *= 0xc4ceb9fe1a85ec53ULL;
	value ^= value >> 32ULL;
	return value;
}

std::string HashTable::Lookup(const uint64_t key)
{
	size_t pos = key % sizeTable;
	HashTableNode* temp = table[pos];
	while (temp)
	{
		if (temp->key == key) {
			return temp->value;
		}
		pos = murmurmix(key + temp->deep + 1) % sizeTable;
		temp = temp->nextTable[pos];
	}
	return std::string();
}

int HashTable::Insert(const uint64_t key, const std::string& val)
{
	int deep = 0;
	size_t pos = key % sizeTable;
	std::vector<HashTableNode*>* tempSave = &table;
	HashTableNode* temp = table[pos];
	while (temp)
	{
		if (temp->key == key) {
			temp->value = val;
			return 0;
		}
		deep++;
		tempSave = &temp->nextTable;
		pos = murmurmix(key + deep) % sizeTable;
		temp = temp->nextTable[pos];
	}
	HashTableNode* newNode = new HashTableNode{ deep, key, val };
	newNode->nextTable.resize(sizeTable);
	(*tempSave)[pos] = newNode;
	return 1;
}

void HashTable::LoadFromFile(const char* filePath)
{
	std::ifstream ifs(filePath);
	if (!ifs.is_open())
	{
		char errMsg[255] = { "\0" };
		strerror_s(errMsg, 255, errno);
		std::cout << "ERROR: Cannot read file " << filePath << " " << errMsg << std::endl;
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
	}

	ifs.close();
	std::cout << "File: " << filePath << " loaded: " << lines << " lines" << std::endl;
}

struct indent
{
	int indention = 0;
	friend std::ostream& operator<<(std::ostream& stream, const indent& val);
};

std::ostream& operator<<(std::ostream& stream, const indent& val) {
	stream << std::string(val.indention * 4, ' ');
	return stream;
}

bool TableHasValues(std::vector<HashTableNode*>& hashTableVector, size_t sizeTable)
{
	for (size_t i = 0; i < sizeTable; i++)
	{
		HashTableNode* temp = hashTableVector[i];
		if (temp != nullptr)
			return true;
	}
	return false;
}

void PrintTableRecursion(std::vector<HashTableNode*>& hashTableVector, size_t sizeTable,
	std::ofstream& ofs, int indention)
{
	for (size_t i = 0; i < sizeTable; i++)
	{
		HashTableNode* temp = hashTableVector[i];
		if (temp)
		{
			ofs << indent(indention) << '"' << std::dec << i << '"' << ": {" << std::endl;

				ofs << indent(indention + 1) << '"' << "deep" << '"' << ": " 
					<< temp->deep << ',' << std::endl;

				ofs << indent(indention + 1) << '"' << "key" << '"' << ": " << '"' << "0x" 
					<< std::uppercase << std::setw(16) << std::hex << std::setfill('0') 
					<< temp->key << '"' << ',' << std::endl;

				ofs << indent(indention + 1) << '"' << "value" << '"' << ": " << '"' 
					<< temp->value << '"' << ',' << std::endl;

				ofs << indent(indention + 1) << '"' << "Table" << '"' << ':';

				if (TableHasValues(temp->nextTable, sizeTable))
				{
					ofs << std::endl;
					ofs << indent(indention + 1) << '{' << std::endl;
					PrintTableRecursion(temp->nextTable, sizeTable, ofs, indention + 2);
					ofs << indent(indention + 1) << '}' << std::endl;
				}
				else
					ofs << " {}" << std::endl;

			ofs << indent(indention) << '}';
			if (i < sizeTable - 1)
				ofs << ',';
			ofs << std::endl;
		}
	}
}

void PrintTable(HashTable& hashT)
{
	std::ofstream ofs("output.json");
	if (!ofs.is_open())
	{
		char errMsg[255] = { "\0" };
		strerror_s(errMsg, 255, errno);
		std::cout << "ERROR: Cannot write file output.json " << errMsg << std::endl;
		return;
	}

	ofs << '{' << std::endl;
		ofs << indent(1) << '"' << "Table" << '"' << ':' << std::endl;
		ofs << indent(1) << '{' << std::endl;
			PrintTableRecursion(hashT.table, hashT.sizeTable, ofs, 2);
		ofs << indent(1) << '}' << std::endl;
	ofs << '}' << std::endl;

	ofs.close();
}

int main()
{
	HashTable hashT;
	hashT.Reserve(10);

	hashT.LoadFromFile("test.txt");

	PrintTable(hashT);

	std::cout << hashT.Lookup(0x9860cec6) << std::endl;

	return 0;
}