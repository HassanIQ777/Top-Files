#include "libutils/src/CLIParser.hpp"
#include "libutils/src/File.hpp"
#include "libutils/src/LoadingBar.hpp"
#include "libutils/src/Log.hpp"
#include "libutils/src/funcs.hpp"
#include "libutils/src/numutils.hpp"
#include "libutils/src/strutils.hpp"
#include <algorithm>
#include <string>
#include <vector>
using funcs::print;

void listfiles_recursive_internal(
    const std::string &dir, const std::vector<std::string> &exception_list,
    std::vector<std::string> &file_list, float min_file_size) {
  if (!fs::exists(dir) || !fs::is_directory(dir))
    return;

  try {
    for (const auto &entry : fs::directory_iterator(dir)) {
      std::string current_path = entry.path().string();

      // 1. Check exceptions
      if (std::find(exception_list.begin(), exception_list.end(),
                    current_path) != exception_list.end()) {
        continue;
      }

      // 2. Distinguish between Files and Directories
      if (fs::is_regular_file(entry.status()) &&
          File::getfilesize(current_path) > min_file_size) {
        file_list.push_back(current_path); // Only add actual files
      } else if (fs::is_directory(entry.status())) {
        // Recurse using the same vector reference
        listfiles_recursive_internal(current_path, exception_list, file_list,
                                     min_file_size);
      }
    }
  } catch (const fs::filesystem_error &) {
    // Log or ignore permission denied errors
  }
}

// Public wrapper function
std::vector<std::string>
listfiles_recursive(const std::string &dir,
                    const std::vector<std::string> &exception_list,
                    float min_file_size) {
  std::vector<std::string> result;
  listfiles_recursive_internal(dir, exception_list, result, min_file_size);
  return result;
}

struct Config {
  std::vector<std::string> exception_list; // exception_list.txt
  size_t min_file_size;

  Config() {
    if (!File::isfile("exception_list.txt")) {
      File::createfile("exception_list.txt");
    }

    if (!File::isfile("config.ini")) {
      File::createfile("config.ini");
      File::appendline("config.ini",
                       "min_file_size=10485760"); // 10 MB is the default
    }
  }

  void load() {
    exception_list = File::readfile("exception_list.txt");
    min_file_size = stoull(File::getFromINI("config.ini", "min_file_size"));
  }
};

int main(int argc, char *argv[]) {
  CLIParser parser(argc, argv);
  if (argc != 2) {
    if (argc == 1) {
      Log::error(true, "Expected 2 arguments, but only one was provided.");
    } else {
      Log::error(true, "Expected 2 arguments, but ", funcs::str(argc),
                 " were provided.");
    }
  }

  std::string home_dir = parser.getArg(1);
  if (!File::isdirectory(home_dir)) {
    Log::error(true, "The provided path is not a directory.");
  }

  Loadingbar::Spinner loading_bar_fetching{
      {"▏", "▎", "▍", "▌", "▋", "▊", "▉", "▊", "▋", "▌", "▍", "▎"},
      150,
      "Fetching files"};

  Config config;
  config.load();
  size_t total_size = 0;

  std::vector<std::string> files = listfiles_recursive(
      home_dir, config.exception_list, config.min_file_size);
  files.erase(std::remove_if(files.begin(), files.end(),
                             [&](const std::string &path) {
                               return !File::isfile(path) ||
                                      File::getfilesize(path) <
                                          config.min_file_size;
                             }),
              files.end());

  loading_bar_fetching.setMsg("Sorting");
  // sort ascendingly
  std::sort(files.begin(), files.end(),
            [](const std::string &a, const std::string &b) {
              return File::getfilesize(a) < File::getfilesize(b);
            });

  loading_bar_fetching.setMsg("Finding total size");
  size_t largest_width = 0;
  for (const auto &file : files) {
    total_size += File::getfilesize(file);
    if (file.size() > largest_width)
      largest_width = file.size();
  }
  loading_bar_fetching.stop();
  print("\r");
  for (const auto &file : files) {
    print(strutils::pad_right(file, largest_width), ": ",
          numutils::bytes(File::getfilesize(file)), "\n");
  }
  if (files.empty()) {
    print("No files were found.\n");
  }

  print("\nShowing diagnosis for \"", home_dir, "\"\n");
  print("---------------------------------\nFound ", files.size(), " files.\n");
  print("Total size: ", numutils::bytes(total_size), "\n");
}