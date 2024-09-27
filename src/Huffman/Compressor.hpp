#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include "../../lib/ThreadPool.hpp"
#include "HuffmanTree.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

#define DEBUG

class Compressor {
private:
  HuffmanTree hf;
  ThreadPoolwithReturn pool;
  int thread_num;
  std::unordered_map<char, std::string> map;

  // 优化：每个线程使用独立的local_map，减少对公用map的竞争，提高并发度
  void get_map(const std::string &s) {
    std::vector<std::future<std::unordered_map<char, int>>> fus;
    fus.resize(thread_num);
    for (int i = 0; i < thread_num; i++) {
      auto size = s.size();
      int start = i * size / thread_num;
      int end = (i + 1) * size / thread_num > size
                    ? size
                    : (i + 1) * size / thread_num;

      std::string sub = s.substr(start, end - start);
      fus[i] = pool.submit([sub = std::move(sub)] {
        std::unordered_map<char, int> local_map;
        for (auto &c : sub) {
          local_map[c]++;
        }
        return local_map;
      });
    }

    std::unordered_map<char, int> global_map;
    for (int i = 0; i < thread_num; i++) {
      auto local_map = fus[i].get();
      for (auto &pair : local_map) {
        global_map[pair.first] += pair.second;
      }
    }

    auto node = hf.buildHuffmanTree(global_map);
    hf.HuffmanMap(node, map, "");
  }

  // 优化2：在每个线程中就把字符串转换为比特流，提高并发性
  void encode(const std::string &s, const std::string &outfilename) {
    using bitstream = std::vector<uint8_t>;
    std::vector<std::future<bitstream>> fus;
    fus.resize(thread_num);
    auto size = s.size();
    for (int i = 0; i < thread_num; i++) {
      int start = i * size / thread_num;
      int end = (i + 1) * size / thread_num > size
                    ? size
                    : (i + 1) * size / thread_num;
      std::string sub = s.substr(start, end - start);
      fus[i] = pool.submit([this, sub = std::move(sub)] {
        std::string outi;
        hf.encode(sub, outi, map);
        auto size = outi.size();
        size_t bitStreamSize = (size + 7) / 8;
        bitstream bitStream(bitStreamSize, 0);
        for (size_t i = 0; i < size; ++i) {
          if (outi[i] == '1') {
            bitStream[i / 8] |= (1 << (7 - (i % 8)));
          }
        }
        return std::move(bitStream);
      });
    }
    bitstream bits;
    for (int i = 0; i < thread_num; i++) {
      auto thread_bitstream = fus[i].get();
      bits.insert(bits.end(), thread_bitstream.begin(), thread_bitstream.end());
    }

    std::ofstream outFile(outfilename, std::ios::binary);
    if (!outFile) {
      throw std::runtime_error("Cannot open file");
    }
    outFile.write(reinterpret_cast<const char *>(bits.data()), bits.size());
    outFile.close();
  }

  void readStringFromFile(const std::string &infilename, std::string &s) {
    std::ifstream in(infilename, std::ios::binary | std::ios::ate);
    if (!in) {
      throw std::runtime_error("Cannot open file");
    }

    // 获取文件大小
    std::streamsize fileSize = in.tellg();
    in.seekg(0, std::ios::beg);

    // 预先分配字符串容量
    s.resize(fileSize);

    // 读取文件内容到字符串
    if (!in.read(&s[0], fileSize)) {
      throw std::runtime_error("Error reading file");
    }
    in.close();
  }

public:
  Compressor(int thread_num) : pool(thread_num), thread_num(thread_num) {}
  Compressor() { thread_num = std::thread::hardware_concurrency(); }
  void compress(const std::string &infilename, const std::string &outfilename) {

#ifdef DEBUG
    std::cout << "Enter function readStringFromFile \n";
    auto start_time = std::chrono::high_resolution_clock::now();
#endif

    std::string s;
    readStringFromFile(infilename, s);

#ifdef DEBUG
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "Time used in readStringFromFile: " << elapsed.count()
              << "s\n";
#endif

    // 将s分段给多个线程读取字符出现频率
#ifdef DEBUG
    std::cout << "Enter function get_map \n";
    start_time = std::chrono::high_resolution_clock::now();
#endif

    get_map(s);

#ifdef DEBUG
    end_time = std::chrono::high_resolution_clock::now();
    elapsed = end_time - start_time;
    std::cout << "Time used in get_map: " << elapsed.count() << "s\n";
#endif

#ifdef DEBUG
    std::cout << "Enter function encode \n";
    start_time = std::chrono::high_resolution_clock::now();
#endif

    encode(s, outfilename);

#ifdef DEBUG
    end_time = std::chrono::high_resolution_clock::now();
    elapsed = end_time - start_time;
    std::cout << "Time used in encode: " << elapsed.count() << "s\n";
#endif
  }

  std::string compress(const std::string &s) {
    // 将s分段给多个线程读取字符出现频率
    get_map(s);
    // 根据map对字符串进行编码
    std::string out;
    for (auto &pair : map) {
      std::cout << pair.first << " " << pair.second << "\n";
    }
    encode(s, out);
    return std::move(out);
  }
};

#endif