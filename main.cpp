#include "libutils/src/CLIParser.hpp"
#include "libutils/src/File.hpp"
#include "libutils/src/LoadingBar.hpp"
#include "libutils/src/Log.hpp"
#include "libutils/src/funcs.hpp"
#include "libutils/src/numutils.hpp"
#include "libutils/src/strutils.hpp"
#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

using funcs::print;

struct FileEntry {
  std::string path;
  uint64_t size;
};

namespace {

void collect_files(const std::string &dir,
                   const std::unordered_set<std::string> &exceptions,
                   std::vector<FileEntry> &out, uint64_t min_file_size) {
  if (!fs::exists(dir) || !fs::is_directory(dir))
    return;

  try {
    for (const auto &entry : fs::directory_iterator(dir)) {
      const std::string current_path = entry.path().string();

      if (exceptions.count(current_path))
        continue;

      // don't follow symlinks
      const auto status = entry.symlink_status();

      if (fs::is_symlink(status)) {
        continue; // nope
      } else if (fs::is_regular_file(status)) {
        const uint64_t size =
            static_cast<uint64_t>(File::getfilesize(current_path));
        if (size >= min_file_size)
          out.push_back({current_path, size});
      } else if (fs::is_directory(status)) {
        collect_files(current_path, exceptions, out, min_file_size);
      }
    }
  } catch (const fs::filesystem_error &) {
    // Permission denied or file vanished mid scan
  }
}

} // namespace

struct Config {
  std::unordered_set<std::string> exception_list; // exception_list.txt
  uint64_t min_file_size = 10485760;              // 10 MB default

  Config() {
    if (!File::isfile("exception_list.txt"))
      File::createfile("exception_list.txt");

    if (!File::isfile("config.ini")) {
      File::createfile("config.ini");
      File::appendline("config.ini", "min_file_size=10485760");
    }
  }

  void load() {
    for (auto &line : File::readfile("exception_list.txt"))
      exception_list.insert(line);

    try {
      min_file_size =
          std::stoull(File::getFromINI("config.ini", "min_file_size"));
    } catch (const std::exception &) {
      // config.ini is empty or missing the key
    }
  }
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    if (argc == 1)
      Log::error(true, "Expected 2 arguments, but only one was provided.");
    else
      Log::error(true, "Expected 2 arguments, but ", funcs::str(argc),
                 " were provided.");
  }

  CLIParser parser(argc, argv);
  std::string home_dir = parser.getArg(1);
  if (!File::isdirectory(home_dir))
    Log::error(true, "The provided path is not a directory.");

  Loadingbar::Spinner loading_bar{
      {"▏", "▎", "▍", "▌", "▋", "▊", "▉", "▊", "▋", "▌", "▍", "▎"},
      150,
      "Fetching files"};

  Config config;
  config.load();

  std::vector<FileEntry> files;
  collect_files(home_dir, config.exception_list, files, config.min_file_size);

  loading_bar.setMsg("Sorting");
  std::sort(
      files.begin(), files.end(),
      [](const FileEntry &a, const FileEntry &b) { return a.size < b.size; });

  loading_bar.setMsg("Finding total size");
  uint64_t total_size = 0;
  size_t largest_width = 0;
  for (const auto &f : files) {
    total_size += f.size;
    largest_width = std::max(largest_width, f.path.size());
  }
  loading_bar.stop();
  print("\r");

  for (const auto &f : files)
    print(strutils::pad_right(f.path, largest_width), ": ",
          numutils::bytes(f.size), "\n");

  if (files.empty())
    print("No files were found.\n");

  print("\nShowing diagnosis for \"", home_dir, "\"\n");
  print("---------------------------------\nFound ", files.size(), " files.\n");
  print("Total size: ", numutils::bytes(total_size), "\n");
}