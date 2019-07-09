#include "ssfi.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

bool is_dir(const string &path)
{
	struct stat path_stat = {0};

	stat(path.c_str(), &path_stat);
	return S_ISDIR(path_stat.st_mode);
}

static void add_file_path_to_queue(const string &file_path) {
	log_debug("Enqueuing: " + file_path);
	gbl.file_queue.push(file_path);
}

static bool search_recursive(const string &path, const regex &regex)
{
	DIR *dir = opendir(path.c_str());
	log_debug("search_recursive: " + path);

	if (NULL == dir) {
		cout << "Failed to list contents of directory: " << path << "\n";
		exit(1);
	}
	struct dirent *entry = NULL;
	bool files_discovered = false;

	while ((entry = readdir(dir))) {
		string filename(entry->d_name);

		/* sanity checks */
		if (filename == "." || filename == "..") {
			continue;
		}

		if (entry->d_type == DT_DIR) {
			files_discovered |= search_recursive(path + "/" + filename, regex);
		} else {
			/* check the extension filter before adding the entry to our list */
			if (1 == matches(regex, filename)) {
				if (!files_discovered) {
					/* first file is to be enqueued; we already have the mutex */
					files_discovered = true;
				} else {
					gbl.file_queue_mutex.lock();
				}

				/* critical section — global file queue */
				add_file_path_to_queue(path + "/" + filename);

				gbl.file_queue_mutex.unlock();
			} else {
				// log_debug(filename + " doesn't match");
			}
		}
	}

	return files_discovered;

	if (0 != closedir(dir)) {
		cout << "Failed to close directory: " << path << "\n";
		exit(1);
	}
}

void search(const string &path, const string &extension_filter)
{
	regex suffix_regex(".*\\." + extension_filter + "$");
	if (!search_recursive(path, suffix_regex)) {
		cout << "Discovered no files to index.\n";
		exit(1);
	}
	gbl.search_complete = true;
}
