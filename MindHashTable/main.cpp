#include <string>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>

template<typename T>
class List
{
public:
	class Item
	{
	public:
		T value;
		int index;
		Item* next;

		Item() : value(), index(0), next(nullptr) {}
		Item(int index) : value(), index(index), next(nullptr) {}

		~Item() { delete value; }

		friend class List;
	};
	Item* head;
	size_t total;

	List() : head(nullptr), total(0) {}

	~List()
	{
		Item* current = head;
		while (current != nullptr) 
		{
			Item* next = current->next;
			delete current;
			current = next;
		}
		head = nullptr;
	}

	T& operator[](int index)
	{
		Item* node = head;
		while (node != nullptr)
		{
			if (node->index == index)
				return node->value;
			node = node->next;
		}

		Item* newItem = new Item(index);
		newItem->next = head;
		head = newItem;
		total++;

		return newItem->value;
	}

	bool hasValues() { return head != nullptr; }
};

struct HashTableNode
{
	int deep;
	uint64_t key;
	std::string value;
	List<HashTableNode*> nextTable;

	HashTableNode() : deep(0), key(0), value("") {}
	HashTableNode(int deep, uint64_t key, std::string value) :
		deep(deep), key(key), value(value) {}
};

class HashTable
{
public:
	size_t sizeTable;
	List<HashTableNode*> table;

	HashTable() : sizeTable(5) {}

	std::string Lookup(const uint64_t key);
	int Insert(const uint64_t key, const std::string& val);
	void LoadFromFile(std::string filePath);
};

uint32_t murmur32scramble(uint32_t k)
{
	k *= 0xcc9e2d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	return k;
}

uint64_t murmurmix(uint64_t value)
{
	uint64_t hash = 0;
	uint32_t* arrHash = (uint32_t*)&hash;
	uint32_t* arrValue = (uint32_t*)&value;
	arrHash[0] = murmur32scramble(arrValue[0] + 1);
	arrHash[1] = murmur32scramble(arrValue[1] + 1);
	return hash;
}

std::string HashTable::Lookup(const uint64_t key)
{
	size_t pos = key % sizeTable;
	HashTableNode* node = table[pos];

	while (node)
	{
		if (node->key == key)
			return node->value;
		pos = murmurmix(key + node->deep + 1) % sizeTable;
		node = node->nextTable[pos];
	}

	return std::string();
}

int HashTable::Insert(const uint64_t key, const std::string& val)
{
	int deep = 0;
	size_t pos = key % sizeTable;
	List<HashTableNode*>* tempSave = &table;
	HashTableNode* node = table[pos];

	while (node)
	{
		if (node->key == key)  {
			node->value = val;
			return 0;
		}
		deep++;
		tempSave = &node->nextTable;
		pos = murmurmix(key + deep) % sizeTable;
		node = node->nextTable[pos];
	}

	HashTableNode* newNode = new HashTableNode(deep, key, val);
	(*tempSave)[pos] = newNode;

	return 1;
}

void HashTable::LoadFromFile(std::string filePath)
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
		if (!line.empty())
		{
			const size_t hashEnd = line.find(" ");

			uint64_t key = 0;
			if (hashEnd == 8)
				key = strtoul(line.c_str(), nullptr, 16);
			else if (hashEnd == 16)
				key = strtoull(line.c_str(), nullptr, 16);
			else
				continue;

			lines += HashTable::Insert(key, line.substr(hashEnd + 1));
		}
	}

	ifs.close();
	std::cout << "File: " << filePath << " loaded: " << lines << " lines" << std::endl;
}

struct indent
{
	int indention = 0;
	friend std::ostream& operator<<(std::ostream& stream, const indent& val);

	indent(int indention) : indention(indention) {}
};

std::ostream& operator<<(std::ostream& stream, const indent& val) {
	stream << std::string(val.indention * 4, ' ');
	return stream;
}

void PrintTableRecursion(List<HashTableNode*>& table, size_t sizeTable,
	std::ofstream& ofs, int indention)
{
	for (size_t i = 0; i < sizeTable; i++)
	{
		HashTableNode* temp = table[i];
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

				if (temp->nextTable.hasValues())
				{
					ofs << std::endl;
					ofs << indent(indention + 1) << '{' << std::endl;
					PrintTableRecursion(temp->nextTable, sizeTable, ofs, indention + 2);
					ofs << indent(indention + 1) << '}' << std::endl;
				}
				else
					ofs << " {}" << std::endl;

			ofs << indent(indention) << '}';
			if (i < table.total - 1)
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
		ofs << indent(1) << '"' << "TableSize" << '"' << ": " << hashT.sizeTable << ',' << std::endl;
		ofs << indent(1) << '"' << "Table" << '"' << ':' << std::endl;
		ofs << indent(1) << '{' << std::endl;
			PrintTableRecursion(hashT.table, hashT.sizeTable, ofs, 2);
		ofs << indent(1) << '}' << std::endl;
	ofs << '}';

	ofs.close();
}

int main()
{
	HashTable hashT;
	size_t sizeTable = 0;
	std::cout << "Table size: ";
	std::cin >> sizeTable;
	if (sizeTable > 0)
		hashT.sizeTable = sizeTable;

	std::string file = "test.txt";
	std::cout << "File to load: ";
	std::cin >> file;
	hashT.LoadFromFile(file);

	std::stringstream ss;
	uint64_t key = 0xbb3dff15;
	std::string keystr = "";
	std::cout << "Press x or X to exit" << std::endl;
	while (true)
	{
		std::cout << "Key: ";
		std::cin >> keystr;
		if (keystr == "x" || keystr == "X")
			break;
		ss << std::hex << keystr;
		ss >> key;
		std::cout << "Returned: " << hashT.Lookup(key) << std::endl;
		ss.clear();
	}

	std::cout << "Generating json" << std::endl;
	PrintTable(hashT);
	std::cout << "Finished json generation" << std::endl;

	std::cout << "Press enter to continue..." << std::endl;
	std::cin.ignore();
	std::cin.get();

	return 0;
}