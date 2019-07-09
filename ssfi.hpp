#ifndef _SSFI_HPP
#define _SSFI_HPP

#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include <pthread.h>
#include <unistd.h>
#include <syscall.h>

#define ensure(function, ...) do { \
		int result = (function)(__VA_ARGS__); \
		if (0 != result) { \
			cout << #function << " failed with return code " << result << "\n"; \
			exit(1); \
		} \
	} while (0)

using std::string;
using std::cout;
using std::ifstream;
using std::mutex;
using std::regex;
using std::vector;
using std::map;
using std::queue;
using std::pair;

struct ssfi_gbl {
	bool debug; /* enables debug logging, though stdout isn't written atomically */
	int n_threads;
	int main_thread_ID;
	string search_path;
	bool search_complete;
	/* queue of file paths to index & accompanying mutex */
	queue<string> file_queue;
	mutex file_queue_mutex;
	/* map of word occurrences & accompanying mutex */
	map<string, int> index;
	mutex index_mutex;
};

const regex WORD_REGEX("[a-zA-Z0-9]+");

/* ssfi.cpp */
extern struct ssfi_gbl gbl;
/* 	returns the number of regex matches for the given string */
int matches(regex re, const string &s);
/* 	writes the given message to stdout if the debug switch was passed at the command line */
/* 	also, appends a newline */
void log_debug(const string &message);

/* search.cpp */
bool is_dir(const string &path);
/* 	if extension_filter is non-empty, only files with the matching suffix will be processed */
/* 	expected format for extension_filter: "txt" to match "*.txt" (periods will not be escaped) */
void search(const string &path, const string &extension_filter);

/* index.cpp */
/* 	function executed by worker threads; performs actual file indexing */
void *index_files(void *_);

#endif
