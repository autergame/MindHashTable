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
#include <unordered_map>

template<typename T> class List
{
public:
	struct Item
	{
		T value;
		int index;
		Item* prev;
		Item* next;

		Item() : value(), index(0), prev(nullptr), next(nullptr) {}
		Item(int index) : value(), index(index), prev(nullptr), next(nullptr) {}

		~Item() { value.~T(); }
	};
	struct ItemBlock
	{
		ItemBlock* next;
	};

	Item* _begin;
	Item* _end;
	Item endItem;
	Item* freeItem;
	ItemBlock* blocks;

    List() : _end(&endItem), _begin(&endItem), freeItem(nullptr), blocks(nullptr)
    {
        endItem.prev = 0;
        endItem.next = 0;
    }

    ~List()
    {
		for (Item* i = _begin, *end = &endItem; i != end; i = i->next)
            i->~Item();
        for (ItemBlock* i = blocks, *next = nullptr; i; i = next)
        {
            next = i->next;
            free(i);
        }
    }
	
	Item* find(T& value)
    {
		for (Item* i = _begin, *end = &endItem; i != end; i = i->next)
            if (i->value == value)
                return i;
        return _end;
    }

	Item* findbyindex(int index)
	{
		for (Item* i = _begin, *end = &endItem; i != end; i = i->next)
			if (i->index == index)
				return i;
		return _end;
	}

	Item* insert(Item* position, int index)
    {
        Item* item = freeItem;
        if (!item)
        {
            size_t allocatedSize = sizeof(ItemBlock) + sizeof(Item);
            ItemBlock* itemBlock = (ItemBlock*)malloc(allocatedSize);
            itemBlock->next = blocks;
            blocks = itemBlock;
            for (Item* i = (Item*)(itemBlock + 1), *end = i + (allocatedSize - sizeof(ItemBlock)) / sizeof(Item); i < end; ++i)
            {
                i->prev = item;
                item = i;
            }
            freeItem = item;
        }

        item = new Item(index);
        freeItem = item->prev;

		Item* insertPos = position;
        if ((item->prev = insertPos->prev))
            insertPos->prev->next = item;
        else
			_begin = item;

        item->next = insertPos;
        insertPos->prev = item;
        return item;
    }

    T& operator[](int index)
    {
		Item* result = findbyindex(index);
		if (result == _end)
			result = insert(result, index);
		return result->value;
    }

	bool isEmpty() { return endItem.prev == 0; }
};

struct HashTableNode
{
	int deep = 0;
	uint64_t key = 0;
	std::string value = "";
    List<HashTableNode*> nextTable;
};

class HashTable
{
public:
	size_t sizeTable = 0;
    List<HashTableNode*> table;

	void Clean(List<HashTableNode*>& table)
	{
		if (table.isEmpty())
			return;
		//Clean(table); // HAS TO BE FOR EACH
		table.~List();
	}

	HashTable(size_t tablesize) : sizeTable(tablesize) {}
	~HashTable() { Clean(table); }

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
    List<HashTableNode*>* tempSave = &table;
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

void PrintTableRecursion(List<HashTableNode*>& hashTableVector, size_t sizeTable,
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

				if (!temp->nextTable.isEmpty())
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
	HashTable hashT(10);

	hashT.LoadFromFile("test.txt");

	std::cout << hashT.Lookup(0xb375545f) << std::endl;

	PrintTable(hashT);

	return 0;
}