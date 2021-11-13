#include <string>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>

template<class T>
class List
{
public:
	struct Item
	{
		T value;
		size_t index;
		Item* next;

		Item() : value(), index(0), next(nullptr) {}
		Item(size_t index) : value(), index(index), next(nullptr) {}

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

	T& operator[](size_t index)
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

//uint64_t scramble(uint64_t value)
//{
//	value *= 0x61757465;
//	value = (value << 19) | (value >> 13);
//	value *= 0x7267616d;
//	return value & 0xFFFFFFFF;
//}
//
//uint64_t keymix2(uint64_t value)
//{
//	uint64_t hash = scramble((value & 0xFFFFFFFFUL) + 1);
//	hash |= (scramble(((value & (0xFFFFFFFFULL << 32)) >> 32) + 1) << 32);
//	return hash;
//}

uint64_t keymix(uint64_t value)
{
	value *= 0x61757465DEADC0DE;
	value = (value << 47) | (value >> 25);
	value *= 0xDEADC0DE7267616d;
	return value;
}

template<class V>
class HashTableNode
{
public:
	uint64_t key;
	V value;
	size_t deep;
	List<HashTableNode<V>*> nextTable;

	HashTableNode() : key(0), value(), deep(0) {}
	HashTableNode(uint64_t key, V value, size_t deep) 
		: key(key), value(value), deep(deep) {}
};

template<class V>
class HashTable
{
public:
	size_t sizeTable;
	List<HashTableNode<V>*> table;

	HashTable() : sizeTable(5) {}
	HashTable(size_t sizeTable) : sizeTable(sizeTable) {}

	V Lookup(const uint64_t key)
	{
		size_t pos = key % sizeTable;
		HashTableNode<V>* node = table[pos];
		while (node)
		{
			if (node->key == key)
				return node->value;
			pos = keymix(key + node->deep + 1) % sizeTable;
			node = node->nextTable[pos];
		}
		return V();
	}

	int Insert(const uint64_t key, const V& val)
	{
		size_t deep = 0;
		size_t pos = key % sizeTable;
		List<HashTableNode<V>*>* tempSave = &table;
		HashTableNode<V>* node = table[pos];
		while (node)
		{
			if (node->key == key)  {
				node->value = val;
				return 0;
			}
			deep++;
			tempSave = &node->nextTable;
			pos = keymix(key + deep) % sizeTable;
			node = node->nextTable[pos];
		}
		(*tempSave)[pos] = new HashTableNode<V>(key, val, deep);
		return 1;
	}
};

void HashTableLoadFromFile(HashTable<std::string>& hashT, std::string filePath)
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

			lines += hashT.Insert(key, line.substr(hashEnd + 1));
		}
	}

	ifs.close();
	std::cout << "File: " << filePath << " loaded: " << lines << " lines" << std::endl;
}

struct indent
{
	size_t indention = 0;
	friend std::ostream& operator<<(std::ostream& stream, const indent& val);

	indent(const size_t indention) : indention(indention) {}
};

std::ostream& operator<<(std::ostream& stream, const indent& val) {
	stream << std::string(val.indention * 4, ' ');
	return stream;
}

void PrintTableRecursion(List<HashTableNode<std::string>*>& table, size_t sizeTable,
	std::ofstream& ofs, size_t indention)
{
	for (size_t i = 0; i < sizeTable; i++)
	{
		HashTableNode<std::string>* temp = table[i];
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

void PrintTable(HashTable<std::string>& hashT)
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

	size_t sizeTable = 5;
	std::cout << "Table size: ";
	std::cin >> sizeTable;
	HashTable<std::string> hashT(sizeTable);

	std::string file = "test.txt";
	std::cout << "File to load: ";
	std::cin >> file;
	HashTableLoadFromFile(hashT, file);

	std::stringstream ss;
	std::string keyStr = "";
	uint64_t key = 0xbb3dff15;
	std::cout << "Press x or X to exit" << std::endl;
	while (true)
	{
		std::cout << "Key: ";
		std::cin >> keyStr;
		if (keyStr == "x" || keyStr == "X")
			break;
		ss << std::hex << keyStr;
		ss >> key;
		ss.clear();
		std::cout << "Returned: " << hashT.Lookup(key) << std::endl;
	}

	std::cout << "Generating json" << std::endl;
	PrintTable(hashT);
	std::cout << "Finished json generation" << std::endl;

	std::cout << "Press enter to continue..." << std::endl;
	std::cin.ignore();
	std::cin.get();

	return 0;
}