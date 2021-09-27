#pragma once

#include <fstream>


class FileUtils {
  private:
    FileUtils() {}

  public:
    static std::string NextLine(std::ifstream& f) {
        std::string line;
        std::getline(f, line);
        return line;
    }

    template <typename T>
    static std::vector<T> NextArray(std::ifstream& f, size_t s) {
        std::vector<T> arr(s);
        for (size_t i = 0; i < s; i++) {
            f >> arr[i];
        }
        return arr;
    }

    // Gets the next non-empty line as an array.
    template <typename T>
    static std::vector<T> NextArray(std::ifstream& f) {
        std::string line;
        std::vector<T> arr;
        while (arr.size() == 0) {
            std::getline(f, line);
            std::istringstream iss(line);
            T val;
            while (iss >> val) {
                arr.push_back(val);
            }
        }
        return arr;
    }
};

