#include <string.h>
#include <cstdlib>
#include <assert.h>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdarg>
#include <iostream>

enum
{
	MAX_K = 4,
	MIN_K = MAX_K / 2
};

enum NodeKind
{
	INTERNAL,
	LEAF
};

typedef struct Node Node;

struct Node
{
	NodeKind kind;
	int keys[MAX_K];
	int length;
	Node *p;
	union
	{
		int values[MAX_K];
		Node *children[MAX_K + 1];
	};
};

Node *root;
int height;

Node *searchLeaf(Node *node, int key) // TODO: refactor
{
	int index = 0;
	while (node->kind != LEAF)
	{
		for (index = 0; index < node->length; index++)
		{
			if (key <= node->keys[index])
			{
				break;
			}
		}
		node = node->children[index];
	}
	return node;
}

void insertLeafData(Node *leaf, int key, int value)
{
	leaf->keys[leaf->length] = key;
	leaf->values[leaf->length] = value;
	leaf->length++;
}

void insertNodeData(Node *node, int key)
{
	node->keys[node->length] = key;
	node->length++;
}

void fixRoot()
{
	Node *right = (Node *)malloc(sizeof(Node));

	if (height >= 1)
	{
		right->kind = INTERNAL;
		memcpy(right->keys, &root->keys[MIN_K + 1], (MIN_K - 1) * sizeof(int));
		memcpy(right->children, &root->children[MIN_K + 1], MIN_K * sizeof(Node *));
		right->length = MIN_K - 1;
	}
	else
	{
		right->kind = LEAF;
		memcpy(right->keys, &root->keys[MIN_K], MIN_K * sizeof(int));
		memcpy(right->children, &root->values[MIN_K], MIN_K * sizeof(int));
		right->length = MIN_K;
	}
	root->length = MIN_K;

	Node *newRoot = (Node *)malloc(sizeof(Node));
	newRoot->kind = INTERNAL;
	newRoot->p = nullptr;
	newRoot->children[0] = root;
	newRoot->children[1] = right;
	newRoot->keys[0] = root->keys[MIN_K];
	newRoot->length = 1;

	root->p = newRoot;
	right->p = newRoot;
	root = newRoot;
	height++;
}

void insertNode(Node *node, int key);

void fixNode(Node *node)
{
	Node *right = (Node *)malloc(sizeof(Node));

	if (node->kind == INTERNAL)
	{
		right->kind = INTERNAL;

		memcpy(right->keys, &node->keys[MIN_K + 1], (MIN_K - 1) * sizeof(int));
		memcpy(right->children, &node->children[MIN_K + 1], MIN_K * sizeof(Node *));
		right->length = MIN_K - 1;
	}
	else
	{
		right->kind = LEAF;

		memcpy(right->keys, &node->keys[MIN_K], MIN_K * sizeof(int));
		memcpy(right->values, &node->values[MIN_K], MIN_K * sizeof(int));
		right->length = MIN_K;
	}
	node->length = MIN_K;
	right->p = node->p;
	node->p->children[node->p->length + 1] = right;
	insertNode(node->p, node->keys[MIN_K]);
}

void insertNode(Node *node, int key)
{
	if (node == root)
	{
		insertNodeData(root, key);

		if (root->length == MAX_K)
		{
			fixRoot();
		}
	}
	else
	{
		insertNodeData(node, key);

		if (node->length == MAX_K)
		{
			fixNode(node);
		}
	}
}

void insertLeaf(int key, int value)
{
	Node *node = searchLeaf(root, key);

	if (height == 0)
	{
		insertLeafData(root, key, value);

		if (root->length == MAX_K)
		{
			fixRoot();
		}
	}
	else
	{
		insertLeafData(node, key, value);

		if (node->length == MAX_K)
		{
			fixNode(node);
		}
	}
}

void insert(int key, int value)
{
	insertLeaf(key, value);
}

void initRoot()
{
	root = (Node *)malloc(sizeof(Node));
	root->kind = LEAF;
	root->length = 0;
	root->p = nullptr;
}

void test()
{
	for (int i = 0; i < 512; i++)
	{
		insert(i, i);
	}
}

void emit(std::ofstream &out, uint8_t byte)
{
	out << byte;
}

void emit32(std::ofstream &out, uint32_t byte4)
{
	emit(out, byte4 & 0xFF);
	emit(out, (byte4 >> 8) & 0xFF);
	emit(out, (byte4 >> 16) & 0xFF);
	emit(out, (byte4 >> 24) & 0xFF);
}

void emitString(std::ofstream &out, const std::string &string)
{
	out << string;
}

void emitNode(std::ofstream &out, Node *node)
{
	if (node == nullptr)
	{
		return;
	}

	emit(out, node->kind);
	emit32(out, node->length);
	if (node->kind == INTERNAL)
	{
		for (int i = 0; i < node->length; i++)
		{
			emit32(out, node->keys[i]);
			emitNode(out, node->children[i]);
		}
		emitNode(out, node->children[node->length]);
	}
	else
	{
		for (int i = 0; i < node->length; i++)
		{
			emit32(out, node->keys[i]);
			emit32(out, node->values[i]); // NOTE: make it take int, double, char, char *
		}
	}
}

void serialize()
{
	assert(root != nullptr);
	std::ofstream out("b_tree.txt", std::ofstream::trunc | std::ofstream::binary);

	emitNode(out, root);
	out.close();
}

uint8_t read(std::ifstream &in)
{
	uint8_t byte;
	in >> byte;
	return byte;
}

uint32_t read32(std::ifstream &in)
{
	uint32_t byte4;
	in.read(reinterpret_cast<char *>(&byte4), sizeof(byte4));
	return byte4;
}

Node *readNode(std::ifstream &in)
{
	Node *n = (Node *)malloc(sizeof(Node));
	n->kind = static_cast<NodeKind>(read(in));
	n->length = read32(in);

	if (n->kind == INTERNAL)
	{
		for (int i = 0; i < n->length; i++)
		{
			n->keys[i] = read32(in);
			n->children[i] = readNode(in);

		}
		n->children[n->length] = readNode(in);
	}
	else
	{
		for (int i = 0; i < n->length; i++)
		{
			n->keys[i] = read32(in);
			n->values[i] = read32(in);
		}
	}
	return n;
}

Node *deserialize()
{
	std::ifstream in("b_tree.txt", std::ifstream::in || std::ifstream::binary);
	assert(in);

	root = readNode(in);

	in.close();
	return root;
}

int main()
{
	initRoot();
	test();

	serialize();
	deserialize();
}