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

  std::vector<std::string> files =
      File::listfiles_recursive(home_dir, config.exception_list);
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