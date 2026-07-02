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
      {"▏", "▎", "▍", "▌", "▋", "▊", "▉"}, 150, "Fetching files"};

  std::vector<std::string> files = File::listfiles_recursive(home_dir);
  files.erase(std::remove_if(
                  files.begin(), files.end(),
                  [](const std::string &path) { return !File::isfile(path); }),
              files.end());

  // sort ascendingly
  std::sort(files.begin(), files.end(),
            [](const std::string &a, const std::string &b) {
              return File::getfilesize(a) < File::getfilesize(b);
            });
  size_t largest_width = 0;
  for (const auto &file : files) {
    if (file.size() > largest_width)
      largest_width = file.size();
  }
  loading_bar_fetching.stop();
  print("\r");
  for (const auto &file : files) {
    print(strutils::pad_right(file, largest_width), ": ",
          numutils::bytes(File::getfilesize(file)), "\n");
  }
}