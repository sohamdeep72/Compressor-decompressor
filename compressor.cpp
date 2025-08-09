#include <iostream>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <bitset>
#include <sstream>
using namespace std;

struct Node {
    char ch;
    int freq;
    Node* left;
    Node* right;
    Node(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
};

struct Compare {
    bool operator()(Node* a, Node* b) {
        return a->freq > b->freq;
    }
};

Node* buildHuffmanTree(unordered_map<char, int>& freqMap) {
    priority_queue<Node*, vector<Node*>, Compare> pq;
    for (auto& pair : freqMap)
        pq.push(new Node(pair.first, pair.second));

    while (pq.size() > 1) {
        Node* l = pq.top(); pq.pop();
        Node* r = pq.top(); pq.pop();
        Node* merged = new Node('\0', l->freq + r->freq);
        merged->left = l;
        merged->right = r;
        pq.push(merged);
    }
    return pq.top();
}

void generateCodes(Node* root, string code, unordered_map<char, string>& huffCode) {
    if (!root) return;
    if (!root->left && !root->right)
        huffCode[root->ch] = code;
    generateCodes(root->left, code + "0", huffCode);
    generateCodes(root->right, code + "1", huffCode);
}

string readFile(string filename) {
    ifstream in(filename, ios::binary);
    string text((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    return text;
}

// Save: [map_size][char][code]\n...[pad_bits][compressed data]
void writeCompressedZip(string binaryStr, unordered_map<char, string>& huffCode, string outFile) {
    ofstream out(outFile, ios::binary);

    // Write map size (number of entries)
    uint32_t mapSize = huffCode.size();
    out.write(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));

    // Write map (char, code length, code)
    for (auto& [ch, code] : huffCode) {
        out.put(ch);
        uint8_t len = code.length();
        out.put(len);
        bitset<256> bits(code);
        for (int i = 0; i < len; i += 8) {
            string byte = code.substr(i, min(8, len - i));
            while (byte.size() < 8) byte += "0";
            bitset<8> b(byte);
            out.put((char)b.to_ulong());
        }
    }

    // Pad and write binary data
    uint8_t extra = binaryStr.size() % 8;
    out.put(extra);
    for (int i = 0; i < binaryStr.size(); i += 8) {
        string byte = binaryStr.substr(i, 8);
        while (byte.size() < 8) byte += "0";
        bitset<8> bits(byte);
        out.put((char)bits.to_ulong());
    }

    out.close();
}

void compress(string inputFile, string outputZip) {
    string text = readFile(inputFile);
    unordered_map<char, int> freqMap;
    for (char ch : text) freqMap[ch]++;
    Node* root = buildHuffmanTree(freqMap);
    unordered_map<char, string> huffCode;
    generateCodes(root, "", huffCode);

    string encoded = "";
    for (char ch : text) encoded += huffCode[ch];
    writeCompressedZip(encoded, huffCode, outputZip);
    cout << "✅ Compression complete. Output: " << outputZip << "\n";
}

// Read: [map_size][char][code]\n...[pad_bits][compressed data]
string readCompressedZip(string filename, unordered_map<string, char>& revMap, uint8_t& extra) {
    ifstream in(filename, ios::binary);
    uint32_t mapSize;
    in.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));

    // Read map
    for (uint32_t i = 0; i < mapSize; ++i) {
        char ch = in.get();
        uint8_t len = in.get();
        string code = "";
        for (int j = 0; j < len; j += 8) {
            char byte = in.get();
            bitset<8> bits(byte);
            code += bits.to_string().substr(0, min(8, len - j));
        }
        revMap[code] = ch;
    }

    // Read padding
    extra = in.get();
    string binaryStr = "";
    char ch;
    while (in.get(ch)) {
        bitset<8> bits(ch);
        binaryStr += bits.to_string();
    }

    // Remove padding
    if (extra > 0)
        binaryStr = binaryStr.substr(0, binaryStr.size() - (8 - extra));

    return binaryStr;
}

void decompress(string zipFile, string outputFile) {
    unordered_map<string, char> revMap;
    uint8_t extraBits;
    string binaryStr = readCompressedZip(zipFile, revMap, extraBits);

    // Decode binary to text
    string curr = "", decoded = "";
    for (char bit : binaryStr) {
        curr += bit;
        if (revMap.count(curr)) {
            decoded += revMap[curr];
            curr = "";
        }
    }

    ofstream out(outputFile, ios::binary);
    out << decoded;
    out.close();
    cout << "✅ Decompression complete. Output: " << outputFile << "\n";
}

int main() {
    int choice;
    cout << "📦 Sohamdeep Huffman Compressor (.sohamdeep format)\n";
    cout << "1. Compress a file\n";
    cout << "2. Decompress a file\n";
    cout << "Enter choice: ";
    cin >> choice;
    cin.ignore();

    if (choice == 1) {
        string inputFile, customFile;
        cout << "Enter input filename: ";
        getline(cin, inputFile);
        cout << "Enter output filename (e.g. result.sohamdeep): ";
        getline(cin, customFile);
        compress(inputFile, customFile);
    } else if (choice == 2) {
        string customFile, outputFile;
        cout << "Enter .sohamdeep filename: ";
        getline(cin, customFile);
        cout << "Enter output filename (e.g. output.txt): ";
        getline(cin, outputFile);
        decompress(customFile, outputFile);
    } else {
        cout << "❌ Invalid choice.\n";
    }

    return 0;
}
