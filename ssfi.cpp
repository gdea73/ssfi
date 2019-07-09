#include "ssfi.hpp"

ssfi_gbl gbl = {};

const string DEBUG_SWITCH = "-d";
const string N_THREADS_SWITCH = "-t";
const int DEFAULT_N_THREADS = 4;
const string TEXT_FILE_EXTENSION = "txt";

void print_usage(const char *program_name)
{
	cout << "usage: " << program_name << " [" << N_THREADS_SWITCH << " [no. worker threads]] " \
		"[path to index] [" << DEBUG_SWITCH << "]\n";
	cout << "Defaults to " << DEFAULT_N_THREADS << " threads if " << N_THREADS_SWITCH
		<< " is unspecified.\n";
	cout << "The path to index must be a directory.\n";

	exit(1);
}

int matches(regex re, const string &s)
{
	std::match_results<string::const_iterator> match;

	std::regex_match(s, match, re);
	return match.size();
}

void log_debug(const string &message)
{
	string thread_info = "";

	/* prepend thread info */
	int thread_ID = syscall(SYS_gettid);
	if (thread_ID == gbl.main_thread_ID) {
		thread_info = "main thread: ";
	} else {
		thread_info = "thread " + std::to_string(thread_ID) + ": ";
	}

	if (gbl.debug) {
		cout << thread_info << message << "\n";
	}
}

static void parse_args(int argc, char **argv, vector<string> &files)
{
	string path;

	if (argc < 2) {
		cout << "Too few arguments specified.\n";
		print_usage(argv[0]);
	}

	for (int i = 1; i < argc; i++) {
		if (string(argv[i]) == N_THREADS_SWITCH) {
			/* attempt to consume the thread number argument */
			i++;
			if (i < argc) {
				gbl.n_threads = atoi(argv[i]);
			}
			/* n_threads is initialized to zero, so the following check will catch invalid input
			 * as well as a lack of input (i.e., i == argc) */
			if (0 == gbl.n_threads) {
				cout << "1 worker thread must be used at minimum.\n";
				print_usage(argv[0]);
			}
		} else if (string(argv[i]) == DEBUG_SWITCH) {
			gbl.debug = true;
		} else if (path.empty()) {
			/* assume this argument is the path to search */
			path = argv[i];
		} else {
			cout << "Unrecognized argument: " << argv[i] << "\n";
			print_usage(argv[0]);
		}
	}

	/* if the n_threads switch was omitted, use the default value */
	if (0 == gbl.n_threads) { gbl.n_threads = DEFAULT_N_THREADS; }

	log_debug("n_threads: " + gbl.n_threads);

	/* ensure the path exists, and is a directory */
	if (!is_dir(path)) {
		cout << "Directory not found: " << path << "\n";
		print_usage(argv[0]);
	}

	gbl.search_path = path;
}

int main(int argc, char **argv)
{
	vector<string> files;

	/* save the ID of the main thread (for logging purposes) */
	gbl.main_thread_ID = syscall(SYS_gettid);

	parse_args(argc, argv, files);

	/* Before creating the worker threads, ensure the main thread holds the lock for the file queue;
	 * this way; worker threads will come awake as soon as files are discovered and enqueued. */
	gbl.file_queue_mutex.lock();

	/* spawn worker threads, which will begin indexing as soon as they can acquire the queue lock */
	pthread_t threads[gbl.n_threads] = {0};
	pthread_attr_t attr = {0};
	ensure(pthread_attr_init, &attr);
	for (int t = 0; t < gbl.n_threads; t++) {
		ensure(pthread_create, &threads[t], &attr, index_files, NULL);
	}

	/* begin searching the directory to index; after the first addition to the queue, the main
	 * thread will release the queue lock to allow worker threads to begin indexing. */
	search(gbl.search_path, TEXT_FILE_EXTENSION);

	/* join all worker threads before exiting from the main thread */
	for (int t = 0; t < gbl.n_threads; t++) {
		ensure(pthread_join, threads[t], NULL);
	}
}
